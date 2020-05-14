#!/usr/bin/env python
# -*- coding: utf-8 -*-
from flask import Flask, render_template, session, request, Response, send_from_directory, make_response
from flask_socketio import SocketIO, emit, disconnect

import os, sys, time, datetime,logging, communication, reviewDB, tocsv
DIR="/home/pi/vprocess5"

logging.basicConfig(filename= DIR + '/log/app.log', level=logging.INFO, format='%(asctime)s:%(levelname)s:%(message)s')

#CREDENCIALES PARA PAGINAS WEB
USER = "user2"
PASSWD = "pass2"

SPEED_MAX = 150 #150 [rpm]
TEMP_MAX  = 50 #130 [C]
TIME_MAX  = 99  #99 [min]

u_set_temp = [SPEED_MAX,0]
u_set_ph   = [SPEED_MAX,SPEED_MAX]

ph_set = [0,0,0,0]
od_set = [0,0,0,0]
temp_set = [0,0,0,0]

rm3     =  0
rm5     =  0
rm_sets = [0,0,0,0,0,0]  #se agrega rm_sets[5] para enviar este al uc
rm_save = [0,0,0,0,0,0]  #mientras que rm_sets[4] se usara solo en app.py para los calculos de tiempo

task = ["grabar", False]
flag_database = False

set_data = [20,0,0,20,0,1,1,1,1,1,0,0,0]
#set_data[8] =: rst2 (reset de bomba2)
#set_data[9] =: rst3 (reset de bomba temperatura)
#set_data[5] =: rst1 (reset de bomba1)
#rm_sets[4]  =: (reset global de bomba remontaje)
#rm_sets[5]  =: (reset local de bomba remontaje)

ficha_producto = [0.0,0.0,0.0,0.0,0.0,"vacio_uchile","vacio_uchile",0,0.0,0,0,0,0] #ficha_producto[9]=set_data[4]:temparatura setpoint
ficha_producto_save = ficha_producto                                  #ficha_producto[10] = set_data[0]: bomba1
                                                                      #ficha_producto[11] = set_data[3]: bomba2
                                                                      #ficha_producto[12] = rm_sets[4]*rm_sets[5] : para saber cuando
                                                                      #enciende y cuando apaga el remontaje y se multiplica por el flujo en base de datos.



# Set this variable to "threading", "eventlet" or "gevent" to test the
# different async modes, or leave it set to None for the application to choose
# the best option based on installed packages.
async_mode = None

#app = Flask(__name__)
app = Flask(__name__, static_url_path="")
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app, async_mode=async_mode)
thread1 = None
error = None

#CONFIGURACION DE PAGINAS WEB
@app.route('/'     , methods=['POST', 'GET'])
@app.route('/login', methods=['POST', 'GET'])
def login():
    global error

    if request.method == 'POST':
        if request.form['username'] != USER or request.form['password'] != PASSWD:
            error = "Credencial Invalida"
            return render_template("login.html", error=error)
        else:
            error='validado'
            return render_template("index.html", error=error)

    error="No Validado en login"
    return render_template("login.html", error=error)




@app.route('/calibrar', methods=['POST', 'GET'])
def calibrar():
    global error

    if request.method == 'POST':
        if request.form['username'] != USER or request.form['password'] != PASSWD:
            error = "Credencial Invalida"
            return render_template("login.html", error=error)
        else:
            error='validado'
            return render_template("calibrar.html", error=error)

    error = 'No Validado en Calibracion'
    return render_template("login.html", error=error)



@app.route('/graphics')
def graphics():
    return render_template('graphics.html', title_html="Variables de Proceso")


@app.route('/dbase', methods=['GET', 'POST'])
def viewDB():
    return render_template('dbase.html', title_html="Data Logger")


@app.route('/remontaje', methods=['GET', 'POST'])
def remontaje():
    global error

    if request.method == 'POST':
        if request.form['username'] != USER or request.form['password'] != PASSWD:
            error = "Credencial Invalida"
            return render_template("login.html", error=error)
        else:
            error='validado'
            return render_template("remontaje.html", error=error)

    error = 'No Validado en Remontaje'
    return render_template('login.html', error=error)




#CONFIGURACION DE FUNCIONES SocketIO
#Connect to the Socket.IO server. (Este socket es OBLIGACION)
@socketio.on('connect', namespace='/biocl')
def function_thread():
    #print "\n Cliente Conectado al Thread del Bioreactor\n"
    #logging.info("\n Cliente Conectado al Thread del Bioreactor\n")

    #Se emite durante la primera conexión de un cliente el estado actual de los setpoints
    emit('Setpoints',           {'set': set_data})
    emit('ph_calibrar',         {'set': ph_set})
    emit('od_calibrar',         {'set': od_set})
    emit('temp_calibrar',       {'set': temp_set})
    emit('u_calibrar',          {'set': u_set_ph})
    emit('u_calibrar_temp',     {'set': u_set_temp})
    emit('power',               {'set': task})
    emit('remontaje_setpoints', {'set': rm_sets, 'save': rm_save })
    emit('producto'           , {'set': ficha_producto, 'save': ficha_producto_save})


    global thread1
    if thread1 is None:
        thread1 = socketio.start_background_task(target=background_thread1)




@socketio.on('power', namespace='/biocl')
def setpoints(dato):
    global task, flag_database
    #se reciben los nuevos setpoints
    task = [ dato['action'], dato['checked'] ]

    #guardo task en un archivo para depurar
    try:
        f = open(DIR + "/task.txt","a+")
        f.write(str(task) + '\n')
        f.close()

    except:
        pass
        #logging.info("no se pudo guardar en realizar en task.txt")


    #Con cada cambio en los setpoints, se vuelven a emitir a todos los clientes.
    socketio.emit('power', {'set': task}, namespace='/biocl', broadcast=True)

    if task[1] is True:
        if task[0] == "grabar":
            flag_database = "True"
            try:
                f = open(DIR + "/flag_database.txt","w")
                f.write(flag_database)
                f.close()

            except:
                pass
                #logging.info("no se pudo guardar el flag_database para iniciar grabacion\n")

        elif task[0] == "no_grabar":
            flag_database = "False"
            try:
                #os.system("rm -rf" + DIR + "/name_db.txt")
                f = open(DIR + "/flag_database.txt","w")
                f.write(flag_database)
                f.close()

            except:
                pass
                #logging.info("no se pudo guardar el flag_database para detener grabacion\n")

        elif task[0] == "reiniciar":
            os.system(DIR + "bash killall")
            os.system("rm -rf" + DIR + "/database/*.db-journal")
            os.system("sudo reboot")

        elif task[0] == "apagar":
            os.system("bash" + DIR + "/killall")
            os.system("sudo shutdown -h now")

        elif task[0] == "limpiar":
            try:
                os.system("rm -rf" + DIR + "/csv/*.csv")
                os.system("rm -rf" + DIR + "/log/*.log")
                os.system("rm -rf" + DIR + "/log/my.log.*")
                os.system("rm -rf" + DIR + "/database/*.db")
                os.system("rm -rf" + DIR + "/database/*.db-journal")

            except:
                pass
                #logging.info("no se pudo completar limpiar\n")




N = None
APIRest = None
@socketio.on('my_json', namespace='/biocl')
def my_json(dato):

    dt  = int(dato['dt'])
    var = dato['var']

    try:
        f = open(DIR + "/window.txt","a+")
        f.write(dato['var'] + ' ' + dato['dt'] +'\n')
        f.close()

    except:
        #print "no se logro escribir la ventana solicitada en el archivo window.txt"
        pass
        #logging.info("no se logro escribir la ventana solicitada en el archivo window.txt")

    #Se buscan los datos de la consulta en database
    try:
        f = open(DIR + "/name_db.txt",'r')
        filedb = f.readlines()[-1][:-1]
        f.close()

    except:
        #print "no se logro leer nombre de ultimo archivo en name_db.txt"
        pass
        #logging.info("no se logro leer nombre de ultimo archivo en name_db.txt")

    global APIRest
    APIRest = reviewDB.window_db(filedb, var, dt)
    socketio.emit('my_json', {'data': APIRest, 'No': len(APIRest), 'var': var}, namespace='/biocl')
    #put files in csv with dt time for samples
    tocsv.csv_file(filedb, dt)


@socketio.on('Setpoints', namespace='/biocl')
def setpoints(dato):
    global set_data
    #se reciben los nuevos setpoints
    set_data = [ dato['alimentar'], dato['mezclar'], dato['ph'], dato['descarga'], dato['temperatura'], dato['alimentar_rst'], dato['mezclar_rst'], dato['ph_rst'], dato['descarga_rst'], dato['temperatura_rst'], dato['alimentar_dir'], dato['ph_dir'], dato['temperatura_dir'] ]

    try:
        set_data[0] = int(set_data[0])   #alimentar
        set_data[1] = int(set_data[1])   #mezclar
        set_data[2] = float(set_data[2]) #ph
        set_data[3] = int(set_data[3])   #descarga
        set_data[4] = int(set_data[4])   #temperatura

        #rst setting
        set_data[5] = int(set_data[5])  #alimentar_rst
        set_data[6] = int(set_data[6])  #mezclar_rst
        set_data[7] = int(set_data[7])  #ph_rst
        set_data[8] = int(set_data[8])  #descarga_rst
        set_data[9] = int(set_data[9])  #temperatura_rst

        #dir setting
        set_data[10]= int(set_data[10]) #alimentar_dir
        set_data[11]= int(set_data[11]) #ph_dir
        set_data[12]= int(set_data[12]) #temperatura_dir

        save_set_data = set_data

    except ValueError:
        set_data = save_set_data #esto permite reenviar el ultimo si hay una exception
        logging.info("exception de set_data")

    #Con cada cambio en los setpoints, se vuelven a emitir a todos los clientes.
    socketio.emit('Setpoints', {'set': set_data}, namespace='/biocl', broadcast=True)

    #guardo set_data en un archivo para depurar
    try:
        settings = str(set_data)
        f = open(DIR + "/setpoints.txt","a+")
        f.write(settings +  time.strftime("Hora__%H_%M_%S__Fecha__%d-%m-%y") + '\n')              #agregar fecha y hora a este string
        f.close()

    except:
        #pass
        logging.info("no se pudo guardar en set_data en setpoints.txt")


#Sockets de calibración de instrumentación
#CALIBRACION DE PH
@socketio.on('ph_calibrar', namespace='/biocl')
def calibrar_ph(dato):
    global ph_set
    #se reciben los parametros para calibración
    setting = [ dato['ph'], dato['iph'], dato['medx'] ]

    #ORDEN DE: ph_set:
    #ph_set = [ph1_set, iph1_set, ph2_set, iph2_set]
    try:
        if setting[2] == 'med1':
            ph_set[0] = float(dato['ph'])   #y1
            ph_set[1] = float(dato['iph'])  #x1

        elif setting[2] == 'med2':
            ph_set[2] = float(dato['ph'])   #y2
            ph_set[3] = float(dato['iph'])  #x2

    except:
        ph_set = [0,0,0,0]

    if (ph_set[3] - ph_set[1])!=0 and ph_set[0]!=0 and ph_set[1]!=0:
        m_ph = float(format(( ph_set[2] - ph_set[0] )/( ph_set[3] - ph_set[1] ), '.2f'))
        n_ph = float(format(  ph_set[0] - ph_set[1]*(m_ph), '.2f'))

    else:
        m_ph = 0
        n_ph = 0

    if ph_set[0]!=0 and ph_set[1]!=0 and ph_set[2]!=0 and ph_set[3]!=0 and m_ph!=0 and n_ph!=0:
        try:
            coef_ph_set = [m_ph, n_ph]
            f = open(DIR + "/coef_ph_set.txt","w")
            f.write(str(coef_ph_set) + time.strftime("__Hora__%H_%M_%S__Fecha__%d-%m-%y") + '\n')
            f.close()
            #acá va el codigo que formatea el comando de calibración.
            communication.calibrate(0,coef_ph_set)

        except:
            pass
            #logging.info("no se pudo guardar en coef_ph_set.txt. Tampoco actualizar los coef_ph_set al uc.")

    #Con cada cambio en los parametros, se vuelven a emitir a todos los clientes.
    socketio.emit('ph_calibrar', {'set': ph_set}, namespace='/biocl', broadcast=True)

    #guardo set_data en un archivo para depurar
    try:
        ph_set_txt = str(ph_set)
        f = open(DIR + "/ph_set.txt","w")
        f.write(ph_set_txt + '\n')
        f.close()

    except:
        pass
        #logging.info("no se pudo guardar parameters en ph_set.txt")



#CALIBRACION OXIGENO DISUELTO
@socketio.on('od_calibrar', namespace='/biocl')
def calibrar_od(dato):
    global od_set
    #se reciben los parametros para calibración
    setting = [ dato['od'], dato['iod'], dato['medx'] ]

    #ORDEN DE: od_set:
    #ph_set = [od1_set, iod1_set, od2_set, iod2_set]
    try:
        if setting[2] == 'med1':
            od_set[0] = float(dato['od'])
            od_set[1] = float(dato['iod'])

        elif setting[2] == 'med2':
            od_set[2] = float(dato['od'])
            od_set[3] = float(dato['iod'])

    except:
        od_set = [0,0,0,0]


    if (od_set[3] - od_set[1])!=0 and od_set[1]!=0:
        m_od = float(format(( od_set[2] - od_set[0] )/( od_set[3] - od_set[1] ), '.2f'))
        n_od = float(format(  od_set[0] - od_set[1]*(m_od), '.2f'))

    else:
        m_od = 0
        n_od = 0

    if od_set[1]!=0 and od_set[3]!=0 and m_od!=0 and n_od!=0:
        try:
            coef_od_set = [m_od, n_od]
            f = open(DIR + "/coef_od_set.txt","w")
            f.write(str(coef_od_set) + time.strftime("__Hora__%H_%M_%S__Fecha__%d-%m-%y") + '\n')
            f.close()

            communication.calibrate(1,coef_od_set)


        except:
            pass
            #logging.info("no se pudo guardar en coef_ph_set en coef_od_set.txt")


    #Con cada cambio en los parametros, se vuelven a emitir a todos los clientes.
    socketio.emit('od_calibrar', {'set': od_set}, namespace='/biocl', broadcast=True)

    #guardo set_data en un archivo para depurar
    try:
        od_set_txt = str(od_set)
        f = open(DIR + "/od_set.txt","w")
        f.write(od_set_txt + '\n')
        f.close()

    except:
        pass
        #logging.info("no se pudo guardar parameters en od_set.txt")




########################## se debe actualizar para los sensores atlas scientific #########################
#CALIBRACIÓN TEMPERATURA
@socketio.on('temp_calibrar', namespace='/biocl')
def calibrar_temp(dato):
    global temp_set
    #se reciben los parametros para calibración
    setting = [ dato['temp'], dato['itemp'], dato['medx'] ]

    #ORDEN DE: od_set:
    #ph_set = [od1_set, iod1_set, od2_set, iod2_set]
    try:
        if setting[2] == 'med1':
            temp_set[0] = float(dato['temp'])
            temp_set[1] = float(dato['itemp'])

        elif setting[2] == 'med2':
            temp_set[2] = float(dato['temp'])
            temp_set[3] = float(dato['itemp'])

    except:
        temp_set = [0,0,0,0]

    if (temp_set[3] - temp_set[1])!=0 and temp_set[0]!=0 and temp_set[1]!=0:
        m_temp = float(format(( temp_set[2] - temp_set[0] )/( temp_set[3] - temp_set[1] ), '.2f'))
        n_temp = float(format(  temp_set[0] - temp_set[1]*(m_temp), '.2f'))

    else:
        m_temp = 0
        n_temp = 0

    if temp_set[0]!=0 and temp_set[1]!=0 and temp_set[2]!=0 and temp_set[3]!=0 and m_temp!=0 and n_temp!=0:
        try:
            coef_temp_set = [m_temp, n_temp]
            communication.calibrate(2,coef_temp_set)

            f = open(DIR + "/coef_temp_set.txt","w")
            f.write(str(coef_temp_set) + time.strftime("__Hora__%H_%M_%S__Fecha__%d-%m-%y") + '\n')
            f.close()

        except:
            pass
            #logging.info("no se pudo guardar en coef_ph_set en coef_od_set.txt")


    #Con cada cambio en los parametros, se vuelven a emitir a todos los clientes.
    socketio.emit('temp_calibrar', {'set': temp_set}, namespace='/biocl', broadcast=True)

    #guardo set_data en un archivo para depurar
    try:
        temp_set_txt = str(temp_set)
        f = open(DIR + "/temp_set.txt","w")
        f.write(temp_set_txt + '\n')
        f.close()

    except:
        pass
        #logging.info("no se pudo guardar parameters en temp_set.txt")



#CALIBRACION ACTUADOR PH
@socketio.on('u_calibrar', namespace='/biocl')
def calibrar_u_ph(dato):
    global u_set_ph
    #se reciben los parametros para calibración
    #setting = [ dato['u_acido_max'], dato['u_base_max'] ]
    try:
        u_set_ph[0] = int(dato['u_acido_max'])
        u_set_ph[1] = int(dato['u_base_max'])

    except:
        u_set_ph = [SPEED_MAX,SPEED_MAX]


    try:
        f = open(DIR + "/umbral_set_ph.txt","w")
        f.write(str(u_set_ph) + '\n')
        f.close()
        communication.actuador(1,u_set_ph)  #FALTA IMPLEMENTARIO EN communication.py

    except:
        pass
        #logging.info("no se pudo guardar umbrales u_set_ph en umbral_set_ph.txt")


    #Con cada cambio en los parametros, se vuelven a emitir a todos los clientes.
    socketio.emit('u_calibrar', {'set': u_set_ph}, namespace='/biocl', broadcast=True)



#CALIBRACION ACTUADOR TEMP
@socketio.on('u_calibrar_temp', namespace='/biocl')
def calibrar_u_temp(dato):
    global u_set_temp
    #se reciben los parametros para calibración

    try:
        u_set_temp[0] = int(dato['u_temp'])
        u_set_temp[1] = 0

    except:
        u_set_temp = [SPEED_MAX,SPEED_MAX]


    try:
        f = open(DIR + "/umbral_set_temp.txt","w")
        f.write(str(u_set_temp) + time.strftime("__Hora__%H_%M_%S__Fecha__%d-%m-%y") + '\n')
        f.close()
        communication.actuador(2,u_set_temp)  #FALTA IMPLEMENTARIO EN communication.py

    except:
        pass
        #logging.info("no se pudo guardar u_set_temp en umbral_set_temp.txt")


    #Con cada cambio en los parametros, se vuelven a emitir a todos los clientes.
    socketio.emit('u_calibrar_temp', {'set': u_set_temp}, namespace='/biocl', broadcast=True)



@socketio.on('producto', namespace='/biocl')
def ficha(dato):
    global ficha_producto

    try:
        ficha_producto[0] = float(dato['cultivo'])
        ficha_producto[1] = float(dato['tasa'])
        ficha_producto[2] = float(dato['biomasa'])
        ficha_producto[3] = float(dato['sustrato'])
        '''
        ficha_producto[4] = float(dato['acidez'])
        ficha_producto[5] = str(dato['fundo'])
        ficha_producto[6] = str(dato['cepa'])
        ficha_producto[7] = str(dato['lote'])
        ficha_producto[8] = str(dato['dosis'])
        '''

        ficha_producto_save = ficha_producto

    except:
        ficha_producto = ficha_producto_save
        logging.info("no se pudo evaluar la ficha de producto")

    socketio.emit('producto', {'set':ficha_producto, 'save': ficha_producto_save}, namespace='/biocl', broadcast=True)
    #communication.zmq_client_data_speak_website(ficha_producto)

    try:
        f = open(DIR + "/ficha_producto.txt","a+")
     	f.write(str(ficha_producto) + '...' + time.strftime("__Hora__%H_%M_%S__Fecha__%d-%m-%y") +'\n')
    	f.close()
        logging.info("se guardo en ficha_producto.txt")

    except:
        #pass
	    logging.info("no se pudo guardar en ficha_producto.txt")



#CONFIGURACION DE THREADS
def background_thread1():
    measures = [0,0,0,0,0,0,0]
    save_set_data = [20,0,0,20,0,1,1,1,1,1,0,0,0]


    while True:
        global set_data, rm_sets, rm_save, ficha_producto, rm3, rm5
        try:
            myList = ','.join(map(str, ficha_producto))
            communication.zmq_client_data_speak_website(myList)    #para database

            ###### se emite setpoint solo si han cambiado!!! ####################################
            for i in range(0,len(set_data)):
                if save_set_data[i] != set_data[i]:
                    communication.cook_setpoint(set_data,rm_sets)
                    save_set_data = set_data
                    #las actualizaciones de abajo deben ir aqui para que aplique la sentencia "!=" en el envio de datos para ficha_producto hacia la Base de Datos
                    ficha_producto[9]  = save_set_data[4]*(1 - save_set_data[9])  #setpoint de temperatura
                    ficha_producto[10] = save_set_data[0]*(1 - save_set_data[5])  #bomba1
                    ficha_producto[11] = save_set_data[3]*(1 - save_set_data[8])  #bomba2


            logging.info("\n ············· SE recalculan los tiempos de remontaje ·············\n")
            ###### se emite setpoint solo si han cambiado!!! ####################################

        except:
            #pass
            logging.info("\n ············· no se pudieron recalcular los tiempos de remontaje ·············\n")


        try:
            #####################################################################################
            #ZMQ DAQmx download data from micro controller + acondicionamiento de variables
            temp_ = communication.zmq_client().split()

            measures[0] = temp_[1]  #Temp_
            measures[1] = 43#temp_[1]  #pH
            measures[2] = 34#temp_[1]  #oD
            measures[3] = 66#temp_[1]  #
            #measures[4] = temp_[1]  #Iod
            #measures[5] = temp_[1]  #
            #measures[6] = temp_[1]  #Iph
            #####################################################################################

            ##logging.info("\n Se ejecuto Thread 1 emitiendo %s\n" % set_data)
            socketio.sleep(0.025)   #probar con 0.05
            logging.info("\n SE ACTUALIZARON LAS MEDICIONES y SETPOINTS \n")


        except:
            #pass
            logging.info("\n NO SE ACTUALIZARON LAS MEDICIONES NI SETPOINTS \n")


        #se termina de actualizar y se emiten las mediciones y setpoints para hacia clientes web.-
        socketio.emit('Medidas', {'data': measures, 'set': set_data}, namespace='/biocl')


if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)
