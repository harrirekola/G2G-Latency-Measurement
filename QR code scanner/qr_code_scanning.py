import argparse
import cv2
from pypylon import pylon
from plotting import plot_latency_measurements
from datetime import datetime
from pyzbar import pyzbar

# Construct the argument parser and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-o", "--output", type=str, default="barcodes.csv",
                help="path to output CSV file containing barcodes")
args = vars(ap.parse_args())

# Initialize the pylon camera
camera = pylon.InstantCamera(pylon.TlFactory.GetInstance().CreateFirstDevice())
camera.Open()

# Set camera parameters
camera.AcquisitionFrameRateEnable.Value = True
camera.AcquisitionFrameRate.Value = 160.0
camera.ExposureTime.Value = 1500.0
camera.MaxNumBuffer = 20

# Start grabbing images from the camera
camera.StartGrabbing(pylon.GrabStrategy_LatestImageOnly)

# Set up the image converter to convert Pylon images to OpenCV-compatible format
converter = pylon.ImageFormatConverter()
converter.OutputPixelFormat = pylon.PixelType_BGR8packed
converter.OutputBitAlignment = pylon.OutputBitAlignment_MsbAligned

csv = open(args["output"], "w")
found = set()
qr_frames = []

while camera.IsGrabbing():
    # Grab the latest image
    grab_result = camera.RetrieveResult(5000, pylon.TimeoutHandling_ThrowException)

    if grab_result.GrabSucceeded():
        # Convert the image to OpenCV format
        image = converter.Convert(grab_result)
        frame = image.GetArray()

        frame = cv2.resize(frame, (190, int(frame.shape[0] * (190 / frame.shape[1]))))

        barcodes = pyzbar.decode(frame)

        for barcode in barcodes:
            # Extract the bounding box location of the barcode and draw the bounding box surrounding the barcode
            (x, y, w, h) = barcode.rect
            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 0, 255), 2)

            # Decode the barcode data
            barcodeData = barcode.data.decode("utf-8")
            barcodeType = barcode.type

            # Draw the barcode data and barcode type on the image
            text = "{} ({})".format(barcodeData, barcodeType)
            cv2.putText(frame, text, (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)

            # If the barcode text is currently not in our CSV file, write the timestamp + barcode to disk and update the set
            if barcodeData not in found:
                csv.write("{},{}\n".format(datetime.now(), barcodeData))
                csv.flush()
                found.add(barcodeData)

            # Store frame number and detection time for QR code data processing
            qr_frames.append({
                'frame_number': barcodeData,  # The barcode data containing the timestamp and frame
                'time': datetime.now()  # The time when the QR code was detected
            })

        # Show the output frame
        cv2.imshow("Barcode Scanner", frame)

        # Break the loop if 'q' key is pressed
        key = cv2.waitKey(1) & 0xFF
        if key == ord("q"):
            break

    # Release the grab result
    grab_result.Release()

csv.close()
cv2.destroyAllWindows()
camera.StopGrabbing()
camera.Close()

plot_latency_measurements("barcodes.csv")