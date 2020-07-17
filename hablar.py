import zmq, time, datetime

tau_zmq_connect     = 0.5  #0.3=300 [ms]
#download data measures with client zmq
def gritando(set_data):
    #Publisher set_data commands
    port = "5556"
    context = zmq.Context()
    socket = context.socket(zmq.PUB)
    socket.bind("tcp://*:%s" % port)
    topic   = 'w'
    #espero y envio valor
    time.sleep(tau_zmq_connect)
    socket.send_string("%s %s" % (topic, set_data))

    return True



def main():
    while 1:
        gritando("wph11.1f111u111m1111t111r111111d111111")
        print "grite"
        time.sleep(0.3)




if __name__ == "__main__":
    main()
