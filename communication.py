#!/usr/bin/env python
# --*- coding: utf-8 -*--
#vprocess5
import sys, zmq, time, logging

DIR="/home/pi/vprocess5"
logging.basicConfig(filename= DIR + '/log/communication.log', level=logging.INFO, format='%(asctime)s:%(levelname)s:%(message)s')



#5556: for download data
#5557: for upload data

#emit command for actuation zmq server (publisher): 'wph14.0feed100unload100mix100temp100rst111111dir111111\n'
#set_data = [ dato['alimentar'], dato['mezclar'], dato['ph'], dato['descarga'], dato['temperatura'], dato['alimentar_rst'], dato['mezclar_rst'], dato['ph_rst'], dato['descarga_rst'], dato['temperatura_rst'], dato['alimentar_dir'], dato['ph_dir'], dato['temperatura_dir'] ]

temp_save_set_data = None
tau_zmq_connect = 0.1 #0.3 [s]: no ha funcionado con menos

SPEED_MAX_MIX = 1500
SPEED_MAX = 150
TEMP_MAX  = 80

T_MAX = 1440
T_MIN = 2

BOMBA_REMONTAJE_T_MAX = T_MAX - 1
BOMBA_REMONTAJE_T_MIN = 1

CICLO_MAX = 20
CICLO_MIN = 1

PH_MIN = 1
PH_MAX = 14

command_save = "vacio"





def calibrate(var, coef):
    #coef[0| =: pendiente;  coef[1] =: intercepto
    #TRAMA: cXs88.88s99.99e ; m=88.888; n=99.99;
    #X = 0,1,2 (ph, od, temp); s = + ó -

    coef[0] = round(coef[0],2)
    coef[1] = round(coef[1],2)

    #sign for coef[0], coef[1]
    if coef[0] < 0:
        coef[0] = -coef[0]
        sign0   = '-'
    else:
        sign0   = '+'

    #sign for coef[1]
    if coef[1] < 0:
        coef[1] = -coef[1]
        sign1   = '-'
    else:
        sign1   = '+'

    #Format for coef[0]
    #Poner cero a la izquierda:
    if coef[0] < 10: #ej. coef[0] = 0.7; coef[0] = 1.1
        coef0text = '0' + str(coef[0]) # => coef[0] = 00.7; coef[0] = 01.1
    #limitar valor
    elif coef[0] >= 100: #aplica para el caso negativo y positivo por que arriba separe el signo.
        coef0text = '99.99'
    #si no es por que tiene formato como: "44.44"
    else:
        coef0text = str(coef[0])

    #Poner cero a la derecha:
    if  coef[0]-int(coef[0]) == 0 and coef[0] < 100:
        coef0text = coef0text + '0'
    elif coef0text[4:] is '':
        coef0text = coef0text + '0'

    #Format for coef[1]
    #Poner cero a la izquierda:
    if coef[1] < 10: #ej. coef[0] = 0.7; coef[0] = 1.1
        coef1text = '0' + str(coef[1]) # => coef[0] = 00.7; coef[0] = 01.1
    #limitar valor
    elif coef[1] >= 100:
        coef1text = '99.99'
    #sino es por que tiene formato 44.44:
    else:
        coef1text = str(coef[1])

    #Poner cero a la derecha:
    if coef[1]-int(coef[1])==0 and coef[1] < 100:
        coef1text = coef1text + '0'
    elif coef1text[4:] is '':
        coef1text = coef1text + '0'

    coef_cook = 'c' + str(var) + sign0 + coef0text + sign1 + coef1text + 'e'

    #guardo coef_cook en un archivo para depurar
    try:
        f = open(DIR + "/coef_m_n.txt","w")
        f.write(coef_cook + '\n')
        f.close()
        #published_setpoint(coef_cook)

    except:
        pass
        #logging.info("no se pudo guardar set de calibrate()")

    return coef_cook


# var = 1 => ph
# var = 2 => temp
# e = end
def actuador(var,u_set):

    if var == 1:
        u_set[0] = int(u_set[0])
        u_set[1] = int(u_set[1])

        #format for u_set[0]
        if u_set[0] >= 10 and u_set[0] < 100:
            u_set_0 = '0' + str(u_set[0])

        elif u_set[0] > 0 and u_set[0] < 10:
            u_set_0 = '00' + str(u_set[0])

        elif u_set[0] >= 100 and u_set[0] <= SPEED_MAX:
            u_set_0 = str(u_set[0])

        else:
            u_set_0 = str(1)


        #format for u_set[1]
        if u_set[1] >= 10 and u_set[1] < 100:
            u_set_1 = '0' + str(u_set[1])

        elif u_set[1] > 0 and u_set[1] < 10:
            u_set_1 = '00' + str(u_set[1])

        elif u_set[1] >= 100 and u_set[1] <= SPEED_MAX:
            u_set_1 = str(u_set[1])

        else:
            u_set_1 = str(1)

        #creation for threshold actuador of ph command
        u_cook = 'u' + str(var) + 'a' + u_set_0 + 'b' + u_set_1 + 'e'


    elif var == 2:
        u_set[0] = int(u_set[0])
        u_set[1] = 0

        #format for u_set[0]
        if u_set[0] >= 10 and u_set[0] < 100:
            u_set_0 = '0' + str(u_set[0])

        elif u_set[0] > 0 and u_set[0] < 10:
            u_set_0 = '00' + str(u_set[0])

        elif u_set[0] >= 100 and u_set[0] <= SPEED_MAX:
            u_set_0 = str(u_set[0])

        else:
            u_set_0 = str(1)

        #creation for threshold actuador of temp command
        u_cook = 'u' + str(var) + 't' + u_set_0 + 'e'

    try:
        f = open(DIR + "/actuador.txt","w")
        f.write(u_cook + '\n')
        f.close()
        #published_setpoint(u_cook);

    except:
        pass
        #logging.info("no se pudo guardar set de actuador()")

    return u_cook




def cook_setpoint(set_data, rm_sets):
    global command_save

    try:
        #format string
        #convert true or false in checkbox from web client to 0 or 1
                    #   rst_bomba1    +      rst_MIX     +    rst_pH        +   rst_bomba2     +   rst_temperatura
        string_rst = str(set_data[5]) + str(set_data[6]) + str(set_data[7]) + str(set_data[8]) + str(set_data[9])
        string_dir = str(set_data[10]) + str(set_data[11]) + str(set_data[12]) + str(set_data[13])
                    #  dir_feed        +      dir_pH       +    dir_Temp       + dir_unload


        #threshold setting:
        #alimentar
        if set_data[0] > SPEED_MAX:     #SPEED_MAX_MIX:
            set_data[0] = SPEED_MAX     #SPEED_MAX_MIX

        elif set_data[0] < 0:
            set_data[0] = 0

        #Mezclar
        if set_data[1] > SPEED_MAX_MIX:  #cambiar a SPEED_MAX_MIX cuando este listo el protocolo que tambien hay que modificar
            set_data[1] = SPEED_MAX_MIX

        elif set_data[1] < 0:
            set_data[1] = 0

        #pH
        if set_data[2] > PH_MAX:
            set_data[2] = PH_MAX

        elif set_data[2] < PH_MIN:
            set_data[2] = PH_MIN

        #descarga
        if set_data[3] > SPEED_MAX:
            set_data[3] = SPEED_MAX

        elif set_data[3] < 0:
            set_data[3] = 0

        #temperatura
        if set_data[4] > TEMP_MAX:
            set_data[4] = TEMP_MAX

        elif set_data[4] < 0:
            set_data[4] = 0


        #format seetting:
        if set_data[0] < 10:
            string_feed = '00' + str(set_data[0])
        elif set_data[0] < 100:
            string_feed = '0'  + str(set_data[0])
        else:
            string_feed = str(set_data[0])


        if set_data[1] < 10:
            string_mix = '000' + str(set_data[1])
        elif set_data[1] < 100:
            string_mix = '00'  + str(set_data[1])
        elif set_data[1] < 1000:
            string_mix = '0'   + str(set_data[1])
        else:
            string_mix = str(set_data[1])


        ph_dec = str(set_data[2]-int(set_data[2]))[1:]
        if ph_dec == '.0':
            if set_data[2] < 10:
                string_ph = '0' + str(set_data[2])
            else:
                string_ph = str(set_data[2])

        else:
            if set_data[2] < 10:
                string_ph = '0' + str(set_data[2])
            else:
                string_ph = str(set_data[2])


        if set_data[3] < 10:
            string_unload = '00' + str(set_data[3])
        elif set_data[3] < 100:
            string_unload = '0'  + str(set_data[3])
        else:
            string_unload = str(set_data[3])


        if set_data[4] < 10:
            string_temp = '00' + str(set_data[4])
        elif set_data[4] < 100:
            string_temp = '0'  + str(set_data[4])
        else:
            string_temp = str(set_data[4])

    except:
        logging.info('\n' + "************** no se pudo construir command setpoints **************"  + '\n')


    try:
        rm_sets[0] = int(rm_sets[0])    #periodo
        rm_sets[1] = int(rm_sets[1])    #rm_duracion
        rm_sets[2] = int(rm_sets[2])    #rm_ciclo
        rm_sets[3] = float(rm_sets[3])  #rm_flujo
        #rm_sets[4] = eneble global de la pagina web
        rm_sets[5] = int(rm_sets[5])    #rm_enable
        #rm_sets[5] = int(rm_sets[4])   #pichicateo para hacer que la bomba funcione directo con la pagina, sin calculo de tiempos

        command = None
        flujo  = 0
        enable = 0

        #### Formateo y limites para variables de remontaje y flujo
        #limites de flujo (ver escala flujemetro manual) rm_sets[3]
        if rm_sets[3] < 0:
            rm_sets[3] = 0
        elif rm_sets[3] >= 3:
            rm_sets[3] = 3

        #str de flujo
        if rm_sets[3] == 3:
            flujo = str(rm_sets[3]) + '.0' #se asegura el decimal para el caso exacto de flujo = 1 o 2 o 3
        else:
            flujo = str(rm_sets[3])
        #flujo = str(rm_sets[3])


        #ajustando flag rm_enable (rm_sets[5])
        if rm_sets[5] is 1:
            rm_sets[5] = 1
        else:
            rm_sets[5] = 0
        #str flag rm_enable
        enable = str(rm_sets[5])


        #vprocess5
        command = 'wph' + string_ph + 'f' + string_feed + 'u' + string_unload + 'm' + string_mix + 't' + string_temp + 'r' + string_rst + 'd' + string_dir + '\n'
        logging.info('\n\n' + command + '\n')
        #wph07.0f033u010m0001t053r111111d000111

    except:
        logging.info('\n' + "************** no se pudo construir command remontaje **************"  + '\n')

    #para respaldo de comandos
    if command != command_save:
        try:
            command_save = command
            f = open(DIR + "/command.txt","a+")
            f.write(time.strftime(" Hora__%H_%M_%S__Fecha__%d-%m-%y") + "   " + command )
            f.close()

        except OSError:
            logging.info('\n' + "no se pudo guardar el command en el archivo de texto" + '\n')

    return command




def main():
    while True:
        pass
        time.sleep(0.05)
        #logging.info("communication.py")


if __name__ == '__main__':
    main()
