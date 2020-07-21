import zmq, time, datetime, os

tau_zmq_while = 0.01 #0.5=500 [ms]
string  = ""
string2 = ""
temporal = ""

def main():

    port_sub = "5557" #'5556'
    context_sub = zmq.Context()
    socket_sub = context_sub.socket(zmq.SUB)
    socket_sub.connect ("tcp://localhost:%s" % port_sub)
    topicfilter = "w"
    socket_sub.setsockopt(zmq.SUBSCRIBE, topicfilter)

    global string, string2, temporal
    i,j = 0,0

    while 1:
        #os.system("clear")
        try:
            string = socket_sub.recv(flags=zmq.NOBLOCK).split()

        except zmq.Again:
            i += 1
            print "MALO numero: %s",i
            pass


        if string != "" and len(string) >= 4:
            temporal = string
            print temporal[0]
            print temporal[1]
            print temporal[2]
            print temporal[3]
            #print temporal[4]
            
            j += 1
            print "BUENO numero: %s",j

            print "--- temporal ---"
            print type(temporal)
            print len(temporal)
            print temporal

            print "\n\n\n"

        time.sleep(tau_zmq_while)


if __name__ == "__main__":
    main()
