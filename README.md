# SCD-TopicServer
Web Socket Topic Server: events notification to client processes subscribed to topic.

## Description

The topic server manage the exchange of message between processes subscribed to a topic. This is a form of interprocess communication. The communication type is one to many: the message sent to topic from one process is sent by server to all processes subscribed to topic.
This server is very simple. The server accept WEB Socket connection from many client, each client can create dynamic topic, which mean that when topic is empty (all client have been unscribed from it, or when all client have disconencted), this topic is delete automatically by server. Each client can send messages to the subscribed topics, or to other topic if the name is known (writer process). 
Eahc client can delete the topic which is subscribed to, as well as can unscribed from it.

This is useful in distribuited sysystem whhere automated processes need to intercomunicate, as a network sensors for example.
The the processes that control the sensors send mesur to topic (writer), and the main control process can collect and process the data sent to topic by each sensor.
