# SCD-TopicServer
Web Socket Topic Server: events notification to client processes subscribed to topic.

## Description

The topic server manage the exchange of message between processes subscribed to a topic. This is a form of interpocess communication. The communication type is one to many: the message sent to topic from one process is sent by server to all processes subscribed to topic.
This server is very simple: accept WEB Socket connection from many client, each client can create dynamic topic, which mean that when topic is empty, when all client have benn unscribed foen it, or when allclient have disconencted form server, this topic is delete automatically byserver. each client can send messages to the topics to which he is subscribed, or if to other topic if the is known (writer process). 
Eahc client cne delete the topic which is subscribed to, as well as can unscribed from it.
