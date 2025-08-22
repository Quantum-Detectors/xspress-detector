import zmq
import json
import argparse
import logging


class LiveViewCombiner(object):
    def __init__(self, pub_port, sub_ports):
        # ZMQ context
        self._context = zmq.Context()

        # Create the publisher
        self._publisher = self._context.socket(zmq.PUB)
        self._publisher.setsockopt(zmq.SNDHWM, 5)
        self._publisher.bind("tcp://0.0.0.0:{}".format(pub_port))
        logging.info("Publisher zmq socket = tcp://0.0.0.0:{}".format(pub_port))

        # Connect our subscriber sockets
        self._subscribers = {}
        for port in sub_ports:
            subscriber = self._context.socket(zmq.SUB)
            subscriber.setsockopt_string(zmq.SUBSCRIBE, "")
            subscriber.setsockopt(zmq.SNDHWM, 5)
            subscriber.connect("tcp://127.0.0.1:{}".format(port))
            self._subscribers[subscriber] = port
            logging.info("Subscriber zmq sockets = tcp://127.0.0.1:{}".format(port))

    def new_frame(self):
        frame = {}
        for port in self._subscribers.values():
            frame[port] = None
        return frame

    def listen(self):
        poller = zmq.Poller()
        for socket in self._subscribers:
            poller.register(socket, zmq.POLLIN)

        current_frame = self.new_frame()
        current_index = 0
        while True:
            socks = dict(poller.poll())
            for socket in self._subscribers:
                if socket in socks and socks[socket] == zmq.POLLIN:
                    message = socket.recv_multipart()
                    header = json.loads(message[0].decode('utf-8'))
                    data = message[1]

                    current_frame[self._subscribers[socket]] = data
                    publish = True
                    for port in current_frame:
                        if current_frame[port] is None:
                            publish = False
                    
                    if publish:
                        header['shape'] = [str(int(header['shape'][0])*int(header['shape'][1])*len(self._subscribers)), header['shape'][2]]
                        new_data = None
                        for index in current_frame:
                            if new_data is None:
                                new_data = current_frame[index]
                            else:
                                new_data = new_data + current_frame[index]
                        header['dsize'] = len(new_data)

                        self._publisher.send_json(header, zmq.SNDMORE)
                        self._publisher.send(new_data)

                        current_frame = self.new_frame()

def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", "--pub_port", default="15510", help="Publish combined image to this port")
    parser.add_argument("-s", "--sub_ports", default="15500,15501,15502,15503,15504,15505,15506,15507,15508", help="Subscribe to these ports")
    args = parser.parse_args()
    return args


def main():
    args = options()

    sub_ports = [int(p.strip()) for p in args.sub_ports.split(",")]
    pub_port = int(args.pub_port)

    combiner = LiveViewCombiner(pub_port, sub_ports)
    logging.info("Initialised the LiveViewCombiner object")
    combiner.listen()


if __name__ == '__main__':
    main()

