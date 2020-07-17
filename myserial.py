import zmq, time, datetime, serial, sys, logging, ports

logging.basicConfig(filename='/home/pi/vprocess5/log/myserial.log', level=logging.INFO, format='%(asctime)s:%(levelname)s:%(message)s')
DIR="/home/pi/vprocess5/"
#5556: for listen data
#5557: for publisher data

tau_serial = 0.35 #[ms]
save_setpoint = "wph11.1f111u111m1111t111r111111d111"


PORT = ports.serial_ports()
ser = serial.Serial(port=PORT, timeout = 1, baudrate = 9600)
logging.info("Conexion Serial al PORT: %s", PORT)


def set_dtr():
    global ser
    ser.close()
    PORT = ports.serial_ports()
    ser = serial.Serial(port=PORT, timeout = 1, baudrate = 9600)
    logging.info("Conexion Serial al PORT: %s", PORT)

    try:
        ser.setDTR(True)
        time.sleep(1)
        ser.setDTR(False)
        time.sleep(1)
        logging.info("======================================================Se ejecuto SET_DTR()======================================")

    except:
        logging.info("---------------------------------------------------- Fallo Ejecucion set_dtr() ---------------------------------")




def rs232():
    global save_setpoint
    #####Listen part: recibe los comandos desde website/app.py para escribir en el uc las acciones: w, etc.
    port_sub = "5556"
    context_sub = zmq.Context()
    socket_sub = context_sub.socket(zmq.SUB)
    socket_sub.connect ("tcp://localhost:%s" % port_sub)
    topicfilter = "w"
    socket_sub.setsockopt(zmq.SUBSCRIBE, topicfilter)

    #####Publisher part: publica las mediciones de sensores obtenidas por serial.
    port_pub = "5557"
    context_pub = zmq.Context()
    socket_pub = context_pub.socket(zmq.PUB)
    socket_pub.bind("tcp://*:%s" % port_pub)
    topic   = 'w'


    flag = False
    while not flag:
        try:
            logging.info("--------------------------- Try Open SerialPort-USB ------------------------------------------------------")
            set_dtr()
            logging.info("Post DTR SET READY: flag = %s", flag)

            if flag:
                ser.write(save_setpoint + '\n')
                result = ser.readline().split()
                logging.info("Primer envio de comando save_setpoint: %s ",  save_setpoint)

            elif not flag:
                logging.info("!!!...Serial communication restart...!!!")
                logging.info("Reenviando ultimo SETPOINT %s", save_setpoint)
                ser.write(save_setpoint + '\n')
                result = ser.readline().split()
                logging.info("not flag: last command: myserial_w_reply_uc: %s ", result)

            try:
                flag = ser.is_open

            except:
                logging.info("*********** No se pudo consultar estado de puerto serial  ***********")

            if flag:
                logging.info('CONEXION SERIAL EXITOSA, entramos al While de lectura y escritura, flag= %s', flag)


            while ser.is_open:
                try:
                    #se consultan cambios de setpoint's
                    try:
                        action = socket_sub.recv(flags=zmq.NOBLOCK).split()[1]
                        if action != "":
                            save_setpoint = action
                            logging.info("****** Se recibe nuevo setpoint desde app.py: %s .... se actualiza save_setpoint: %s ******", action, save_setpoint)
                    #de no haberlos se continua enviando el ultimo setpoint
                    except zmq.Again:
                        action = save_setpoint
                        logging.info("____________ zmq.Again except ____________")

                    #escribiendo y leyendo al uc_sensores:
                    logging.info("Enviando al uc_sensores: %s  ", action)
                    ser.write(action + '\n')
                    SERIAL_DATA = ser.readline().split()

                    if len(SERIAL_DATA) > 2:
                        #enviamos la data serial a speaker para su publicacion por zmq en el puerto 5557
                        socket_pub.send_string("%s %s" % (topic, SERIAL_DATA))
                        logging.info("********* Se Recojen mediciones del UC_SENSORES: %s *******\n\n", SERIAL_DATA)

                    else:
                        logging.info("********* Sin respuesta correcta SERIAL_DATA: %s *******\n\n", SERIAL_DATA)


                except:
                    logging.info("no se pudo escribir al uc")


                time.sleep(tau_serial)

        except serial.SerialException:
            print "conexion serial no realizada"
            logging.info("Sin Conexion Serial")
            flag = False
            time.sleep(2)
            set_dtr() #esta sobrando esto al parecer.

        time.sleep(tau_serial)

    logging.info("Fin de myserial.py")
    return True


def main():
    rs232()


if __name__ == "__main__":
    main()
