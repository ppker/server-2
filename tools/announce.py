#############################
# announce.py
# Send a server message to all characters, in all zones, across all processes.
#
# Usage
# python3 announce.py "Here is a message from python!"
#
# Requirements
# pip3 install zmq pyzmq
#
#############################

import socket
import sys
import zmq
import struct

# ENUM: Send to every character, in every zone, on every map process
MSG_CHAT_SERVMES = 6

# Message offset (This changed to 0x17 from 0x18 in Nov 2021, adjust accordingly!)
MESSAGE_OFFSET = 0x17

context = zmq.Context()
sock = context.socket(zmq.DEALER)

ip_str = "127.0.0.1"
port = 54003

ip_bytes = socket.inet_aton(ip_str)
(ip_int,) = struct.unpack("!I", ip_bytes)
ipp = ip_int | (port << 32)
ipp_bytes = struct.pack("!Q", ipp)

print(f"Connecting to {ip_str}:{port} ({ipp})")

sock.setsockopt(zmq.ROUTING_ID, ipp_bytes)
sock.connect("tcp://127.0.0.1:54003")


def print_help():
    print("You must provide a message to send.")
    print("Example:")
    print('python3 .\\announce.py "Here is a message from python!"')


def build_chat_packet(msg_type, gm_flag, zone, sender, msg):
    buff_size = min(236, MESSAGE_OFFSET + len(msg))
    buffer = bytearray(buff_size)

    if sender is None:
        sender = ""

    buffer[0] = 0x17
    buffer[1] = 0x82

    buffer[4] = msg_type

    if gm_flag == True or gm_flag == 1:
        buffer[5] = 0x01

    buffer[6] = zone

    struct.pack_into("{}s".format(len(sender)), buffer, 0x08, bytes(sender, "utf-8"))
    struct.pack_into(
        "{}s".format(len(msg)), buffer, MESSAGE_OFFSET, bytes(msg, "utf-8")
    )

    return buffer


def send_server_message(msg):
    print(f"Sending '{msg}'")
    buffer = build_chat_packet(MSG_CHAT_SERVMES, 1, 0, "", msg)
    sock.send_multipart([buffer], zmq.NOBLOCK)


def main():
    if len(sys.argv) < 2:
        print_help()
        return

    send_server_message(sys.argv[1])


if __name__ == "__main__":
    main()
