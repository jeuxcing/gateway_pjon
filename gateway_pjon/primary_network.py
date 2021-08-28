"""
Communication between the primary and the secondaries.

Part of the primary.
"""

import logging
import time
from threading import Thread
from collections import deque

import zmq


logger = logging.getLogger('root')


class PrimaryNetwork(Thread):
    """Network communication for the primary."""

    def __init__(self):
        """Constructor for the class."""
        Thread.__init__(self)

        self.ctx = zmq.Context()
        self.poller = zmq.Poller()

        self._running = True
        logger.debug("new primary network")

        self.configure_socket_from_secondaries()
        self.configure_socket_to_secondaries()

        self.messages_to_secondaries = deque([])
        self.arduino_messages = deque([])
        self.status_messages = deque([])
        time.sleep(2)  # needed to wait for the slave slow connection
        self.socket_to_secondaries.send_multipart([b"100", b"init"])

    def configure_socket_from_secondaries(self):
        """Create ZMQ socket listening to the primary."""
        self.socket_from_secondaries = self.ctx.socket(zmq.SUB)
        self.socket_from_secondaries.bind("tcp://0.0.0.0:5557")
        self.socket_from_secondaries.setsockopt(zmq.SUBSCRIBE, b'')

        self.poller.register(self.socket_from_secondaries, zmq.POLLIN)

    def configure_socket_to_secondaries(self):
        self.socket_to_secondaries = self.ctx.socket(zmq.PUB)
        self.socket_to_secondaries.bind("tcp://0.0.0.0:5556")

    def run(self):
        """
        Run loop of the class.

        Send messages to primary if any and listen to
        communications from the primary (populating the inbox queue).

        We use a poller to get a timeout while listening,
        this way we can kill the loop and send messages without waiting for the next message.
        """
        while self._running:
            self.send_command()
            self.receive()
        logger.info("end NetworkCommunication")

    def stop(self):
        """Stop the main loop."""
        self._running = False

    def receive(self):
        """Receive messsages from secondaries.

        We have two sockets, one for arduinos messages
        and one for secondaries statuses (connections, error...)
        """
        sockets = dict(self.poller.poll(5))
        if not sockets:
            return

        msg = self.socket_from_secondaries.recv_multipart()  # we only have one listening socket
        msg = [s.decode() for s in msg]

        logger.info("from arduinos -- message: {}".format(msg))

        # button from arduino
        try:
            device_id, message_string = msg
        except Exception as e:
            logger.error("Unknown message from arduino : {}".format(msg))
            logger.exception(e)
            return
        self.arduino_messages.append((device_id, message_string))

    def send_command(self):
        """Send a command to the secondaries, with a channel and a message."""
        while self.messages_to_secondaries:
            logger.info(f"message len before: {len(self.messages_to_secondaries)}")
            msg = self.messages_to_secondaries.popleft()
            message = [s.encode() for s in msg]
            self.socket_to_secondaries.send_multipart(message)
            logger.info(f"sending to arduinos : {message}")
            logger.info(f"message len after: {len(self.messages_to_secondaries)}")
