import cv2
import mediapipe as mp
import time
import numpy as np
import math
import sys
import os

# Use webcam 0
cap = cv2.VideoCapture(0)
# Set up the size
wCam, hCam = 1280, 720
cap.set(3, wCam)
cap.set(4, hCam)


# Get the directory where this script is located
script_dir = os.path.dirname(os.path.abspath(__file__))
model_path = os.path.join(script_dir, 'hand_landmarker.task')

base_options = mp.tasks.BaseOptions(model_asset_path=model_path)
options = mp.tasks.vision.HandLandmarkerOptions(
    base_options=base_options,
    running_mode=mp.tasks.vision.RunningMode.VIDEO,
    num_hands = 2,
    min_hand_detection_confidence = 0.7
)
mpHands = mp.tasks.vision.HandLandmarker.create_from_options(options)

# For getting hand connections
HAND_CONNECTIONS = mp.tasks.vision.HandLandmarksConnections.HAND_CONNECTIONS

# Set up for showing fps
pTime = 0
cTime = 0


press = False
start_press = None
hold_printed = False


METHOD_CODES = {
    "position": 1,
    "startHold": 2,
    "holding": 3,
    "endHold": 4,
    "click": 5
}


def send_output(output_type, cx, cy, length, x1, y1, z1, x2, y2, z2):
    code = METHOD_CODES[output_type]
    output = f"{code} {cx} {cy} {length:.2f} {x1} {y1} {z1:.4f} {x2} {y2} {z2:.4f}"
    print(output)
    sys.stdout.flush()

while True:
    success, img = cap.read()
    imgRGB = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    
    # Convert to image
    mp_image = mp.Image(mp.ImageFormat.SRGB, imgRGB)
    # Get image dimension
    h, w, c = img.shape

    timestamp_ms = int(time.time() * 1000)

    result = mpHands.detect_for_video(mp_image, timestamp_ms)


    # If detect hand
    if (result.hand_landmarks) :
        # For each hand
        for handLms in result.hand_landmarks :
            lmList = []
            # For each landmark
            for id, lm in enumerate(handLms) :
                # Convert normalized coordinates to pixel coordinates
                cx, cy = int(lm.x * w), int(lm.y * h)
                cz = lm.z
                lmList += [[id, cx, cy, cz]]
                
                cv2.circle(img, (cx, cy), 5, (255, 0, 255), cv2.FILLED)
                cv2.putText(img, str(id), (cx + 10, cy), cv2.FONT_HERSHEY_COMPLEX, 0.5, (255, 0, 255), 2)
            
            #print(lmList[4], lmList[8])
            x1, y1, z1 = lmList[4][1], lmList[4][2], lmList[4][3]
            x2, y2, z2 = lmList[8][1], lmList[8][2], lmList[8][3]
            cx, cy = (x1 + x2) // 2, (y1 + y2) // 2

            length = math.hypot(x2 - x1, y2 - y1)

            # Draw connections between landmarks
            for connections in HAND_CONNECTIONS :
                # For each connection get the first and second node
                start_lm = handLms[connections.start]
                end_lm = handLms[connections.end]

                # Convert normalized coordinates to pixel coordinates
                start_point = (int(start_lm.x * w), int(start_lm.y * h))
                end_point = (int(end_lm.x * w), int(end_lm.y * h))

                cv2.line(img, start_point, end_point, (0, 255, 0), 2)

            cv2.circle(img, (x1, y1), 10, (255, 255, 255), cv2.FILLED)
            cv2.circle(img, (x2, y2), 10, (255, 255, 255), cv2.FILLED)

            # Detect click or hold
            cv2.line(img, (x1, y1), (x2,y2), (255, 255, 255), 3)
            if length <= 100 :
                cv2.circle(img, (cx, cy), 5, (255, 0, 255), cv2.FILLED)
                if press == False :
                    press = True
                    start_press = time.time()
                elif hold_printed == True :
                    send_output("holding", cx, cy, length, x1, y1, z1, x2, y2, z2)
                else :
                    # Check if held for more than 0.3 seconds
                    if not hold_printed and (time.time() - start_press >= 0.3) :
                        send_output("startHold", cx, cy, length, x1, y1, z1, x2, y2, z2)
                        hold_printed = True
            else :
                if press == True :
                    end_press = time.time()
                    duration = end_press - start_press
                    if duration < 0.3 :
                        send_output("click", cx, cy, length, x1, y1, z1, x2, y2, z2)
                    press = False
                    start_press = None
                elif hold_printed == True :
                    send_output("endHold", cx, cy, length, x1, y1, z1, x2, y2, z2)
                    hold_printed = False
                else :
                    send_output("position", cx, cy, length, x1, y1, z1, x2, y2, z2)

                
            cv2.putText(img, str(length), (cx, cy), cv2.FONT_HERSHEY_COMPLEX, 0.5, (255, 8, 255), 2)


    # Show fps calculated
    cTime = time.time()
    fps = 1/(cTime - pTime)
    pTime = cTime
    cv2.putText(img, str(int(fps)), (10, 70), cv2.FONT_HERSHEY_COMPLEX, 2, (255, 8, 255), 3)

    cv2.imshow("Image", img)
    cv2.waitKey(1)
