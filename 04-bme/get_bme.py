import argparse
import time

import matplotlib.pyplot as plt
import serial
import serial.tools.list_ports


def read_value(ser):
	while True:
		line = ser.readline().decode('ascii', errors='ignore').strip()
		try:
			return float(line)
		except ValueError:
			continue


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
	parser.add_argument("--port", default="COM7")
	parser.add_argument("--time", type=float, default=30.0)
	args = parser.parse_args()

	try:
		ser = serial.Serial(port=args.port, baudrate=115200, timeout=1.0)
	except serial.SerialException as error:
		print(f"Could not open {args.port}: {error}")
		print("Close PuTTY/VS Code serial monitor if it is using this port.")
		print_ports()
		return

	print(f"Port {ser.name} opened")

	measure_ts = []
	measure_temp_C = []
	measure_pres_Pa = []
	measure_hum_percent = []

	start_ts = time.time()

	try:
		while True:
			ts = time.time() - start_ts
			if ts > args.time:
				break

			ser.write("temp\n".encode('ascii'))
			temp_C = read_value(ser)
			ser.write("pres\n".encode('ascii'))
			pres_Pa = read_value(ser)
			ser.write("hum\n".encode('ascii'))
			hum_percent = read_value(ser)

			measure_ts.append(ts)
			measure_temp_C.append(temp_C)
			measure_pres_Pa.append(pres_Pa)
			measure_hum_percent.append(hum_percent)

			print(f"{temp_C:.2f} C - {pres_Pa:.1f} Pa - {hum_percent:.1f}% - {ts:.2f}s")
			time.sleep(0.5)
	finally:
		ser.close()
		print("Port closed")

	plt.subplot(3, 1, 1)
	plt.plot(measure_ts, measure_temp_C)
	plt.title("BME280 temperature")
	plt.xlabel("time, s")
	plt.ylabel("temperature, C")

	plt.subplot(3, 1, 2)
	plt.plot(measure_ts, measure_pres_Pa)
	plt.title("BME280 pressure")
	plt.xlabel("time, s")
	plt.ylabel("pressure, Pa")

	plt.subplot(3, 1, 3)
	plt.plot(measure_ts, measure_hum_percent)
	plt.title("BME280 humidity")
	plt.xlabel("time, s")
	plt.ylabel("humidity, %")

	plt.tight_layout()
	plt.show()


if __name__ == "__main__":
	main()
