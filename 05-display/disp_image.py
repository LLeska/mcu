import argparse
import time

import serial
import serial.tools.list_ports
from PIL import Image


def print_ports():
	ports = list(serial.tools.list_ports.comports())
	if len(ports) == 0:
		print("No COM ports found")
		return
	print("Available ports:")
	for port in ports:
		print(f"  {port.device} - {port.description}")


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("--port", default="COM3")
	parser.add_argument("--image", default="pics/get.jpg")
	parser.add_argument("--delay", type=float, default=0.0005)
	args = parser.parse_args()

	image = Image.open(args.image).convert("RGB").resize((320, 240))

	try:
		ser = serial.Serial(port=args.port, baudrate=115200, timeout=0.1)
	except serial.SerialException as error:
		print(f"Could not open {args.port}: {error}")
		print("Close PuTTY/VS Code serial monitor if it is using this port.")
		print_ports()
		return

	print(f"Port {ser.name} opened")
	try:
		for y in range(240):
			for x in range(320):
				r, g, b = image.getpixel((x, y))
				color = (r << 16) | (g << 8) | b
				ser.write(f"disp_px {x} {y} {color:06x}\n".encode("ascii"))
				if args.delay > 0:
					time.sleep(args.delay)
			print(f"line {y + 1}/240")
	finally:
		time.sleep(0.1)
		ser.close()
		print("Port closed")


if __name__ == "__main__":
	main()
