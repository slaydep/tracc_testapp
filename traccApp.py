import threading
import serial
import sys
from threading import Thread

last_received = ''
baudrate=9600
port="COM7"
hilos=[]
baud_rate=["50","75","110","134","150","200","300","600","1200","1800","2400", "4800", "9600", "14400", "19200", "28800", "38400", "57600", "115200"]
salida=True

def menu1():
    print("\n\n***traccApp***")
    print("puerto: {}, baudrate {}".format(port,baudrate))
    print("1. Conectar con arduino.")
    print("2. Cambiar puerto serie.")
    print("3. Cambiar baudrate.")
    print("4. Salir.")

def menu2():

    print("\n\n***Arduino***")
    print("1. Salir.")
    print("2. Ultimo dato recibido.")

def iniciar_serial():
    try:
        ser = serial.Serial(
                port=port,
                baudrate=baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1,
                xonxoff=0,
                rtscts=0,
                interCharTimeout=None
            )
        return ser
    except:
        print("No se pudo iniciar el puerto serial")
        return None

def recibiendo(ser):
    global last_received
    global salida
    buffer = ''
    print("hilo 1")
    if ser!=None:
        print("hilo en ejecucion")
        while salida:
            # last_received = ser.readline()
            buffer += ser.read(ser.inWaiting()).decode()
            if '\n' in buffer:
                buff_=buffer.split('\n')
                last_received, buffer = buffer.split('\n')[-2:]
                last_received=last_received.replace("\r","")
                #print("Maquina: {}".format(last_received))

def enviando():
    pass

def arduino():
    global last_received
    global salida
    salida=True
    ser=iniciar_serial()
    opcion=True
    if ser!=None:
        hilo1=threading.Thread(target=recibiendo, args=(ser,), daemon=True)
        hilo1.start()
        while opcion:
            menu2()
            opcion=input("Digite la opcion: ")
            if opcion=="1":
                salida=False
                opcion=False
            if opcion=="2":
                print("Ultimo dato: ", last_received)



def camb_puerto():
    entrada=input("""Escriba el puerto (ejem. "COM7"): """)
    if entrada[:3]=="COM":
        return entrada
        # port=entrada
        # print("puerto: {}".format(port))
    else:
        print("no se cambió el puerto")
        return None

def camb_baudrate():
    try:
        entrada=input("""Escriba la velocidad del puerto (ejem. "9600"): """)
        if entrada in baud_rate:
            return int(entrada)
            #baudrate=int(entrada)
            #print("puerto cambiado a ",baudrate)
        else:
            print("Seleccione una de las velocidades tipicas")
            print(", ".join(baud_rate))
            return None

    except:
        print("puerto incorrecto.")

    



if __name__ == '__main__':
    while True:
        menu1()
        try:
            entrada=int(input("Digite una opcion: "))
            if entrada==1:
                arduino()
            if entrada==2:
                port=camb_puerto()
            if entrada==3:
                baudrate=camb_baudrate()
            if entrada==4:
                break
                
        except:
            print("opcion no valida....")
        
