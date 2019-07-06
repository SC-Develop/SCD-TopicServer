# SCD-TopicServer
Web Socket Topic Server: events notification to client processes subscribed to topic.

## Description

The topic server manage the exchange of message between processes subscribed to a topic. This is a form of interprocess communication. The communication type is one to many: the message sent to topic from one process is forwarded by server to all processes subscribed to topic.
This server is very simple. The server accept WEB Socket connection from many client, each client can create dynamic topic, which mean that when topic is empty (all client have been unscribed from it, or when all client have disconencted), this topic is delete automatically by server. Each client can send messages to the subscribed topics, or to other topic if the name is known (writer process). 
Each client can delete the topic which is subscribed to, as well as can unscribed from it.
Note the each client process can be reader or writer or both, this depend from implementation of yur own system. 
The role of client is not established from server but from architecture of your own application.
Also, the server can manage the static topic. At this stage, static topic can be only added manually from admin to the topic file, and can not be deleted automatically by server but only manually.

### When is it useful?

This topic server is suitable in an automated distribuited multiprocess/multithreading system where each processes/thread need to notifyn some data to each other, where each process needs to know what the other processes are doing, as in a intelligent sensors network for example.
You can use this topic server also for use as very simple chat.
