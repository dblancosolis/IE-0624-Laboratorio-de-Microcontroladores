import serial
import time
import csv
import pandas as pd
import matplotlib.pyplot as plt

ser = serial.Serial('/dev/pts/3', 9600)  
counter = 0

data_file = "dataCollecter.csv"

# Archivo CSV para escritura
with open(data_file, "w", newline="") as csvfile:
    csv_writer = csv.writer(csvfile)
    csv_writer.writerow(["Tiempo (s)", "Setpoint", "Input", "Output"])  # Encabezado
    
    try:
        while True:
            counter += 0.5  
            
            # Leer desde puerto serial
            line = ser.readline().decode().strip()
            setpoint, input_val, output = map(float, line.split(","))
            
            # Escribir en el archivo CSV
            csv_writer.writerow([counter, setpoint, input_val, output])
            
            # Datos en terminal
            print(f"[{counter}] {setpoint}, {input_val}, {output}")
    
    except KeyboardInterrupt:
        ser.close()
        print("Conexión cerrada.")

# Leer datos desde el CSV
data = pd.read_csv(data_file)

# Extraer las columnas
tiempo = data.iloc[:, 0]  # Primera columna 
setpoint = data.iloc[:, 1]  # Segunda columna
input_val = data.iloc[:, 2]  # Tercera columna 
output = data.iloc[:, 3]  # Cuarta columna 

# Graficar Setpoint y guardarlo como PNG
plt.figure(figsize=(8, 6))
plt.plot(tiempo, setpoint, label="Setpoint", color='r')
plt.xlabel("Tiempo (s)")
plt.ylabel("Setpoint")
plt.title("Temperatura de operacion del sistema")
plt.legend()
plt.grid(True)
plt.savefig("grafica_opt.png")  # Guardar gráfica de Setpoint como PNG
plt.close()  # Cerrar la gráfica para evitar que se superpongan

# Graficar Input y guardarlo como PNG
plt.figure(figsize=(8, 6))
plt.plot(tiempo, input_val, label="Input", color='g')
plt.xlabel("Tiempo (s)")
plt.ylabel("Input")
plt.title("Temperatura sensada del sistema")
plt.legend()
plt.grid(True)
plt.savefig("grafica_temp.png")  # Guardar gráfica de Input como PNG
plt.close()  # Cerrar la gráfica para evitar que se superpongan

# Graficar Output y guardarlo como PNG
plt.figure(figsize=(8, 6))
plt.plot(tiempo, output, label="Output", color='b')
plt.xlabel("Tiempo (s)")
plt.ylabel("Output")
plt.title("Señal de control del sistema")
plt.legend()
plt.grid(True)
plt.savefig("grafica_ctrl.png")  # Guardar gráfica de Output como PNG
plt.close()  # Cerrar la gráfica para evitar que se superpongan
