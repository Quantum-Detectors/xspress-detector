from typing import Tuple, List

class ListModeIPPortGen:
    IP_PREFIX = "192.168.0."
    BASE_IP = 65
    IP_STEP = 4
    BASE_PORT = 30125
    MAX_PORT = 30134
    NUM_PORTS_PER_IP = 10

    @classmethod
    def config_for_1_process(cls):
        """
        If only a single process, we can listen to incomming packets from any ip on the 10 ports
        """
        return "0.0.0.0", list(range(cls.BASE_PORT, cls.MAX_PORT+1))


    def __init__(self, num_endpoints_per_process, num_process):
        self.ip_last = self.BASE_IP
        self.port = self.BASE_PORT
        if self.NUM_PORTS_PER_IP % num_endpoints_per_process != 0:
            raise ValueError(f"num_endpoints must be a factor of {self.NUM_PORTS_PER_IP}")
        self.n = num_endpoints_per_process
        self.current_count = 0
        self.num_process = num_process
    
    def __iter__(self):
        return self

    def __next__(self):
        if self.num_process <= 0:
            raise StopIteration
        self.num_process -= 1

        ip = self.IP_PREFIX + str(self.ip_last)
        ports = []
        for i in range(self.n):
            port_number = self.BASE_PORT + self.current_count
            if port_number > self.MAX_PORT:
                raise ValueError(f"logic bug. port number can't be bigger than {self.MAX_PORT}")
            ports.append(self.BASE_PORT + self.current_count)
            self.current_count += 1
        if self.current_count  >= self.NUM_PORTS_PER_IP:
            self.ip_last += self.IP_STEP
        self.current_count =  self.current_count % self.NUM_PORTS_PER_IP
        return ip, ports

def get_x3x2_list_mode_addresses(num_cards: int, use_tcp_relay: bool) -> List[Tuple[str, List[int]]]:
    """Get the list of addresses for the frame receivers to connect to for X3X2 list mode

    This will return the addresses of the Xspress cards if connecting directly or the
    addresses of the TCP relay server applications (1 per card).

    Args:
        num_cards (int): Number of cards in the Xspress system
        use_tcp_relay (bool): False to connect directly to Xspress or True
                              to connect to the TCP relay application

    Returns:
        List[Tuple[str, List[int]]]: List of addresses as [(ip, [port]),...]
    """
    if not use_tcp_relay:
        return [(f"192.168.0.{card_num+1}", [30125]) for card_num in range(1, num_cards+1)]
    else:
        return [("127.0.0.1", [13000 + card_num]) for card_num in range(1, num_cards+1)]
