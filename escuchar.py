import zmq, time, datetime, os

tau_zmq_while_write = 0.01 #0.5=500 [ms]
string = ""

def main():

    #####Listen part: recibe los comandos desde website/app.py para escribir en el uc las acciones: w, etc.
    port_sub = "5557" #'5556'
    context_sub = zmq.Context()
    socket_sub = context_sub.socket(zmq.SUB)
    socket_sub.connect ("tcp://localhost:%s" % port_sub)
    topicfilter = "w"
    socket_sub.setsockopt(zmq.SUBSCRIBE, topicfilter)

    global string
    while 1:
        #os.system("clear")
        try:
            string = socket_sub.recv(flags=zmq.NOBLOCK).split()

        except zmq.Again:
            pass


        if string != "" and len(string) > 2:
            '''
            a = string[1].split()
            b = a[0]
            c = b[2:7]
            print float(c)
            '''
            try:
                z = float( string[1].split()[0][2:7] )

            except:
                pass
                
            print type(z)
            print z
            print "\n\n\n"

        time.sleep(tau_zmq_while_write)




if __name__ == "__main__":
    main()
