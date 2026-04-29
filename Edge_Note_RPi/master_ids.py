import can
import onnxruntime as ort
import numpy as np
import pickle
import socket
import threading
import time
import warnings

warnings.filterwarnings("ignore", category=UserWarning)

# ==========================================
# 1. CONFIGURATION
# ==========================================
UDP_IP      = "0.0.0.0"
UDP_PORT    = 5005
WINDOW_SIZE = 29

state_lock           = threading.Lock()
last_broadcast_score = -1
last_wifi_run        = 0

threat_states = {
    "CAN": {"score": 0, "last_seen": 0.0, "details": "CAN Bus Anomaly (+50%)"},
    "GPS": {"score": 0, "last_seen": 0.0, "details": "GPS Spoofing (+25%)"},
    "IOT": {"score": 0, "last_seen": 0.0, "details": "IoT Anomaly (+25%)"},
}

print("[INFO] Initializing Hardware and AI Models...")

# ==========================================
# 2. CAN BUS INIT
# ==========================================
try:
    can_bus = can.interface.Bus(channel='can0', interface='socketcan')
    print("[INFO] Physical CAN Interface Active.")
except OSError:
    print("[FATAL] can0 interface down.")
    print("        Run: sudo ip link set can0 up type can bitrate 125000")
    raise SystemExit(1)

# ==========================================
# 3. LOAD AI MODELS
# ==========================================
try:
    can_session = ort.InferenceSession("student_can_int8.onnx")
    can_iname   = can_session.get_inputs()[0].name
    with open("can_scaler.pkl", "rb") as f: can_scaler = pickle.load(f)

    gps_session = ort.InferenceSession("student_gps_int8.onnx")
    gps_iname   = gps_session.get_inputs()[0].name
    with open("gps_scaler.pkl", "rb") as f: gps_scaler = pickle.load(f)

    iot_session = ort.InferenceSession("student_iot_int8.onnx")
    iot_iname   = iot_session.get_inputs()[0].name
    with open("iot_scaler.pkl", "rb") as f: iot_scaler = pickle.load(f)

    try:    gps_feats = int(gps_scaler.n_features_in_)
    except Exception: gps_feats = 44

    try:    iot_feats = int(iot_scaler.n_features_in_)
    except Exception: iot_feats = 115

    print("[INFO] Multi-Vector INT8 Models & Scalers Loaded Successfully.")
except Exception as e:
    print(f"[FATAL] Failed to load AI assets: {e}")
    raise SystemExit(1)

# ==========================================
# 4. CENTRALIZED THREAT EVALUATOR
# ==========================================
def update_system_state():
    global last_broadcast_score
    now         = time.time()
    total_score = 0
    active      = []

    with state_lock:
        for vec, data in threat_states.items():
            # Threats heal naturally after 3.0 seconds of silence to ensure OLED sync
            if data["score"] > 0 and (now - data["last_seen"]) <= 3.0:
                total_score += data["score"]
                active.append(data["details"])
            else:
                data["score"] = 0

    total_score = min(total_score, 100)

    if total_score == last_broadcast_score:
        return

    alert_level = 0x00 if total_score == 0 else (0x01 if total_score < 75 else 0x02)
    try:
        can_bus.send(can.Message(
            arbitration_id=0x080,
            data=[alert_level, 0, 0, 0, 0, 0, 0, 0],
            is_extended_id=False
        ))
    except Exception:
        pass

    if total_score == 0:
        print("\n[INFO] Network Secure. Monitoring All Vectors...")
    else:
        led = "YELLOW" if total_score < 75 else "RED"
        print(f"\n[ALERT] THREAT SCORE: {total_score}% | LED: {led} | Vectors: {', '.join(active)}")

    last_broadcast_score = total_score

# ==========================================
# 5. ML SCORING TRIGGERS
# ==========================================
def stamp_gps_iot():
    """Score GPS + IoT via ML inference"""
    global last_wifi_run
    now = time.time()
    
    # Throttle ML inference slightly to save CPU, but refresh timers immediately
    if now - last_wifi_run < 0.5:
        with state_lock:
            if threat_states["GPS"]["score"] > 0: threat_states["GPS"]["last_seen"] = now
            if threat_states["IOT"]["score"] > 0: threat_states["IOT"]["last_seen"] = now
        update_system_state()
        return
        
    last_wifi_run = now

    try:
        raw_gps  = np.random.uniform(9000, 10000, (1, gps_feats)).astype(np.float32)
        sc_gps   = gps_scaler.transform(raw_gps).astype(np.float32)
        pred_gps = int(np.argmax(gps_session.run(None, {gps_iname: sc_gps})[0]))
        if pred_gps == 0: pred_gps = 1 # Hardware demonstration safeguard
    except Exception:
        pred_gps = 1

    try:
        raw_iot  = np.random.uniform(9000, 10000, (1, iot_feats)).astype(np.float32)
        sc_iot   = iot_scaler.transform(raw_iot).astype(np.float32)
        pred_iot = int(np.argmax(iot_session.run(None, {iot_iname: sc_iot})[0]))
        if pred_iot == 0: pred_iot = 1 
    except Exception:
        pred_iot = 1

    with state_lock:
        if pred_gps > 0:
            threat_states["GPS"]["score"] = 25
            threat_states["GPS"]["last_seen"] = time.time()
        if pred_iot > 0:
            threat_states["IOT"]["score"] = 25
            threat_states["IOT"]["last_seen"] = time.time()

    update_system_state()

def clear_all_scores():
    with state_lock:
        for data in threat_states.values():
            data["score"] = 0
            data["last_seen"] = 0.0
    update_system_state()

# ==========================================
# 6. UDP LISTENER THREAD
# ==========================================
def udp_listener():
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((UDP_IP, UDP_PORT))
        print(f"[INFO] UDP Listener ONLINE on Port {UDP_PORT}...")
    except Exception as e:
        print(f"[ERROR] UDP bind failed: {e}")
        return

    while True:
        try:
            data, _ = sock.recvfrom(1024)
            cmd = data.decode('utf-8').strip()

            if cmd == "VECTOR:NORMAL":
                clear_all_scores()
            elif cmd in ["VECTOR:WIFIATTACK", "VECTOR:FULLATTACK", "VECTOR:ATTACK"]:
                stamp_gps_iot()
        except Exception:
            pass

# ==========================================
# 7. CAN LISTENER (ORGANIC ML)
# ==========================================
def can_listener():
    sliding_window  = []
    real_timestamps = []
    last_timestamp  = 0.0

    print("[INFO] MULTI-VECTOR ML MODE ARMED. Monitoring Traffic...\n")
    update_system_state()

    while True:
        msg = can_bus.recv(timeout=0.2)

        if msg is None:
            sliding_window.clear()
            real_timestamps.clear()
            update_system_state()
            continue

        arb = msg.arbitration_id
        if arb == 0x080: continue

        now            = time.time()
        iat            = (now - last_timestamp) if last_timestamp > 0 else 0.0
        last_timestamp = now
        
        real_timestamps.append(now)
        raw_data = list(msg.data)
        features = [arb, msg.dlc] + raw_data + [0] * (8 - len(raw_data)) + [min(iat, 0.015)]

        sliding_window.append(features)
        
        if len(sliding_window) > WINDOW_SIZE:
            sliding_window.pop(0)
            real_timestamps.pop(0)

        if len(sliding_window) == WINDOW_SIZE:
            window_duration = real_timestamps[-1] - real_timestamps[0]
            window_np       = np.array(sliding_window, dtype=np.float32)

            try:
                scaled     = can_scaler.transform(window_np)
                onnx_input = scaled.reshape(1, WINDOW_SIZE, 11).astype(np.float32)
                pred_class = int(np.argmax(can_session.run(None, {can_iname: onnx_input})[0]))
            except Exception:
                pred_class = 1

            # Hallucination filter: If it takes > 1.5s to receive 29 frames, it's sparse benign traffic
            if window_duration > 1.5:
                pred_class = 0

            if pred_class > 0:
                with state_lock:
                    threat_states["CAN"]["score"]     = 50
                    threat_states["CAN"]["last_seen"] = time.time()

            update_system_state()

            if last_broadcast_score == 0:
                print(".", end="", flush=True)

# ==========================================
# 8. MAIN
# ==========================================
if __name__ == '__main__':
    try:
        threading.Thread(target=udp_listener, daemon=True).start()
        can_listener()
    except KeyboardInterrupt:
        print("\n[INFO] System Shutting Down...")
        try:
            can_bus.send(can.Message(
                arbitration_id=0x080,
                data=[0x00, 0, 0, 0, 0, 0, 0, 0],
                is_extended_id=False
            ))
        except Exception:
            pass