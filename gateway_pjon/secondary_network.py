"""
Communication stuff between master and slave raspi.

Slave side of the channel.

We use zmq for the communication.
"""

import logging
from threading import Thread

import zmq

logger = logging.getLogger("root")


class SecondaryNetwork(Thread):
    """
    Network communication class between primary and secondary.

    Contains 2 network channel :
      * one SUB to get messages from the Primary
      * one PUB to send messages to the Primary

    The Primary binds to 2 two sockets so we can have all
    Secondarys using only 2 sockets to speak to the primary.
    """

    def __init__(
        self,
        primary_adress: "str",
        inbox: "dict of iterable",
        outbox: "iterable",
        topics: "iterable" = None,
    ):
        """
        Constructor for the class.

        Arguments:
         * inbox: messages from the Primary, used for secondary and / or devices
         * outbox: messages to send to the Primary
         * topics: the topics to subscribe to. If any, we listen to everything
        """
        Thread.__init__(self)

        self.topics = topics or [b""]

        self.primary_adress = primary_adress

        self.ctx = zmq.Context()
        self.poller = zmq.Poller()
        self.configure_socket_in()
        self.configure_socket_out()

        self.messages_from_primary = inbox
        self.messages_to_primary = outbox

        self._running = True
        logger.debug("new network")

    def run(self):
        """
        Run loop of the class.

        Send messages to primary if any and listen to
        communications from the primary (populating the inbox queue).

        We use a poller to get a timeout while listening,
        this way we can kill the loop and send messages without waiting for the next message.
        """
        while self._running:
            self.send_messages_to_primary()
            self.receive_messages_from_primary()
        logger.info("end NetworkCommunication")

    def stop(self):
        """Stop the primary loop."""
        self._running = False

    def configure_socket_in(self):
        """Create ZMQ socket listening to the primary."""
        server_adress_in = f"tcp://{self.primary_adress}:5556"

        self.sock_in = self.ctx.socket(zmq.SUB)
        self.sock_in.connect(server_adress_in)

        for topic in self.topics:
            self.sock_in.setsockopt(zmq.SUBSCRIBE, topic)
        self.poller.register(self.sock_in, zmq.POLLIN)

    def configure_socket_out(self):
        """Create ZMQ socket emitting to the primary."""
        server_adress_out = f"tcp://{self.primary_adress}:5557"
        self.sock_out = self.ctx.socket(zmq.PUB)
        self.sock_out.connect(server_adress_out)

        server_adress_statuses_out = f"tcp://{self.primary_adress}:5558"
        self.sock_out_statuses = self.ctx.socket(zmq.PUB)
        self.sock_out_statuses.connect(server_adress_statuses_out)

    def send_messages_to_primary(self):
        """Send messages in the outbox to the primary."""
        while self.messages_to_primary:
            msg = self.messages_to_primary.popleft()
            msg = [s.encode() for s in msg]
            if b"status" in msg[0]:
                logger.debug(f"sending status : {msg}")
                self.sock_out_statuses.send_multipart(msg)
            else:
                logger.debug(f"sending {msg} to primary")
                self.sock_out.send_multipart(msg)

    def receive_messages_from_primary(self):
        """Receive messages from the primary and populate the inbox for the correct device."""
        sockets = dict(self.poller.poll(2))
        if not sockets:
            return
        msg = (
            self.sock_in.recv_multipart()
        )  # we only have one listening socket
        msg = [s.decode() for s in msg]

        logger.info(f"received {msg} from primary")
        device_id, message_string = msg
        self.messages_from_primary[device_id].append(message_string)
