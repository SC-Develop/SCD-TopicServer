/**
 * @class SCDTopicServer https://github.com/sc-develop/
 *
 * @brief SCD Topic Server
 *
 *        SCD Topic Sever is a Web Socket Server which allow you to notify events to all client processes
 *        subscribed to the same topic.
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com
 *
 * @copyright (c) 2019 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 *
 */

#include "scdtopicserver.h"

#include <QStringList>
#include <QRegularExpression>
#include <QFile>

/**
 * @brief SCDTopicServer::SCDTopicServer constructor
 * @param parent
 */
SCDTopicServer::SCDTopicServer(QObject *parent, int port) : QWebSocketServer("SCD Topic Server", QWebSocketServer::NonSecureMode, parent), port(port)
{
   commands.insert(TMK, "TMK");
   commands.insert(TDL, "TDL");
   commands.insert(TRC, "TRC");
   commands.insert(TUC, "TUC");
   commands.insert(TRN, "TRN");
   commands.insert(TSM, "TSM");

   connect(this,SIGNAL(newConnection()),this,SLOT(onNewConnection()));
}

/**
 * @brief SCDTopicServer::~SCDTopicServer
 */
SCDTopicServer::~SCDTopicServer()
{

}

/**
 * @brief SCDTopicServer::start start SCDImg server
 */
int SCDTopicServer::start()
{
   loadTopicList(); // load topic list, if fail the list is empty, there are no topic.

   unregisterAllSubscriptions();

   if (listen(QHostAddress::Any,port))
   {
      lastErrorMsg = "Server is listening on port " + QString::number(port) + " for incoming connections...";
      qDebug() <<  lastError();
      return 1;
   }

   lastErrorMsg = "Unable to start server on port " + QString::number(port) + this->errorString();
   qDebug() << lastErrorMsg;
   return 0;
}

/**
 * @brief SCDTopicServer::stop
 * @return
 */
void SCDTopicServer::stop()
{
   close();

   // unregisterAllSubscriptions();
}

/**
 * @brief SCDTopicServer::onNewConnection
 */
void SCDTopicServer::onNewConnection()
{
   QWebSocket *socket = nextPendingConnection();

   connect(socket, SIGNAL(textMessageReceived(QString)),this,SLOT(onTextMessageReceived(QString)));
   // connect(socket, SIGNAL(binaryMessageReceived(QByteArray)),this,SLOT(onbinaryMessageReceived(QByteArray)));
   connect(socket, SIGNAL(aboutToClose()),this,SLOT(onAboutToClose()));
   connect(socket, SIGNAL(disconnected()),this,SLOT(onDisconnected()));
   connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(onSocketError(QAbstractSocket::SocketError)));

   QString socketId = addressToString(socket->peerAddress(),socket->peerPort());

   QString hexHash = addressToHex(socket->peerAddress(),socket->peerPort());

   qDebug() << "New Web Socket Connection: " << socketId << hexHash;

   sockList[hexHash] = socket;

   maxHeaderSize = 1024;
}

/**
 * @brief SCDTopicServer::onTextMessageReceived
 * @param message
 */
void SCDTopicServer::onTextMessageReceived(QString message)
{
   QWebSocket *socket = static_cast<QWebSocket *>(sender());

   qDebug() << "Received: " + message;

   Command command;

   int headerSize = readHeader(message,command);

   if (headerSize)
   {
      int ret = 0;

      QString topic      = header[commands[command]].toString();
      QString hexAddress = addressToHex(socket);
      QString strAddress = addressToString(socket);
      QString notifyMsg;

      QStringList subscribers;

      switch (command)
      {
         case TMK: // make a new topic

           notifyMsg = "TMK|" + topic;

           qDebug() << notifyMsg + ": " + topic;
           qDebug() << "ID: " + strAddress;

           ret = addTopic(topic); // ret => 0,1,2

         break;

         case TDL: // delete a topic
         {
            notifyMsg = "TDL|" + topic;

            qDebug() <<  notifyMsg + ": " + topic;

            ret  = removeTopic(topic,subscribers);            
         }
         break;

         case TRN:
         case TRC: // register a client to topic

           notifyMsg = ( (command==TRN) ? "TRN|":"TRC|") + topic;

           qDebug() << "Register client '" +  hexAddress + "' to topic '" + topic + "'";

           ret = subscribeToTopic(topic,hexAddress,command==TRN);

         break;

         case TUC: // unscribe a client from topic

           notifyMsg = "TUC|" + topic;

           qDebug() << notifyMsg + ": " + topic;

           ret = unscribeFromTopic(topic,hexAddress);

         break;

         case TSM: // send a message to topic

           message.remove(0,headerSize+1);

           qDebug() << "[" + hexAddress + "] Message to " + topic << " => " << message;

           ret = sendMessageToTopic(topic,message,hexAddress);

           notifyMsg = "TSM|" + topic;

         break;
      }

      notifyMsg = "[server@notify]:" + notifyMsg; // prepend notify header to message

      QString result = QString::number(ret);

      notifyMsg += "|" + result + "|" + lastErrorMsg + "\n";

      socket->sendTextMessage(notifyMsg);

      if (!subscribers.isEmpty())
      {
         sendMessageToSubscribers(subscribers,notifyMsg,hexAddress);
      }

      /*if (ret>0)
      {
         notifyMsg += "|" + result;

         if (ret==2) // warning
         {
            notifyMsg += "|" + lastErrorMsg;
         }

         notifyMsg += "\n";

         socket->sendTextMessage(notifyMsg);

         if (!subscribers.isEmpty())
         {
            sendMessageToSubscribers(subscribers,notifyMsg,hexAddress);
         }
      }
      else
      {

      }*/
   }
}

/**
 * @brief WebSocketHandler::disconnected exit from event thread loop (thread end)
 */
void SCDTopicServer::onDisconnected()
{
   QWebSocket *socket = static_cast<QWebSocket *>(sender());

   qDebug() << "Client disconnected:" + addressToString(socket);

   delete socket;
}

/**
 * @brief WebSocketHandler::onSocketError
 */
void SCDTopicServer::onSocketError(QAbstractSocket::SocketError error)
{
   QWebSocket *socket = static_cast<QWebSocket *>(sender());

   qDebug() << error << errorString() << addressToString(socket);

   socket->abort();
}

/**
 * @brief SCDTopicServer::onAboutToClose
 */
void SCDTopicServer::onAboutToClose()
{
   QWebSocket *socket = static_cast<QWebSocket *>(sender());

   qDebug() << "Client about to disconnected:" + addressToString(socket);

   QString hashIndex = addressToHex(socket);

   sockList.remove(hashIndex);

   unscribeFromTopics(hashIndex);
}

/**
 * @brief SCDTopicServer::onBinaryMessageReceived
 * @param message
 */
/*void SCDTopicServer::onBinaryMessageReceived(QByteArray message)
{

}*/

/**
 * @brief SCDTopicServer::addressToHex
 * @param socket
 * @return
 */
QString SCDTopicServer::addressToHex(QWebSocket *socket)
{
   return addressToHex(socket->peerAddress(),socket->peerPort());
}

/**
 * @brief SCDTopicServer::addressToString
 * @param socket
 * @return
 */
QString SCDTopicServer::addressToString(QWebSocket *socket)
{
   return addressToString(socket->peerAddress(),socket->peerPort());
}

/**
 * @brief SCDTopicServer::checkHdrField
 * @param headerItem
 * @param cmd
 * @return
 */
int SCDTopicServer::checkHeaderField(const QString &headerItem, SCDTopicServer::Command &cmd)
{
   QStringList field = headerItem.split(":"); // split into field name and value pair

   if (field.size() != 2)
   {
      lastErrorMsg = "Bad header item:" + headerItem;
      return 0;
   }

   QString name = field.at(0).trimmed();

   if (commands.contains(name))
   {
      header.insert(name,field.at(1).trimmed());

      cmd = static_cast<enum Command>(commands.indexOf(QRegularExpression(name)));

      return 1;
   }

   return 0;
}

/**
 * @brief WebSocketHandler::checkHeaderField split header intem anf get field name and valu,
 *                                           if field name match fieldName param add it to header map and return 1
         *                                   else return 0;
 * @param headerItem header item in format <field name>:<value>
 * @param fieldName
 * @return 1 on success, 0 un failure
 */
int SCDTopicServer::checkHeaderField(const QString &headerItem, const QString &fieldName)
{
   QStringList field = headerItem.split(":"); // split into field name and value pair

   if (field.size() != 2)
   {
      lastErrorMsg = "Bad header item:" + headerItem;
      return 0;
   }

   QString name = field.at(0).trimmed();

   if (name==fieldName)
   {
      header.insert(name,field.at(1).trimmed());
      return 1;
   }

   lastErrorMsg = "Bad header field name:" + headerItem;

   return 0;
}

/**
 * @brief WebSocketHandler::readHeader read headers lines and fill header map.
 *        Call this method only when statu=WAITFORHEADER.
 *        when header read is complete the stustus change to WAITFORDATA.
 *        on error return 0, otherwise return 1.
 *        Do not use the sequence '|' (sharp) char into complete file path
 *
 *        SCDTMH (one line fast header struct)
 *
 *          TMK command => SCDTMH:1.0\tTMK:<topic name>\n          // Topic MaKe   => make a new topic
 *          TDL command => SCDTMH:1.0\tTDL:<topic name>\n          // Topic DeLete => delete a topic
 *          TRN command => SCDTMH:1.0\tTRN:<topic name>\n          // Topic Register New  => register a client to topic, create the topic if not exists
 *          TRC command => SCDTMH:1.0\tTRC:<topic name>\n          // Topic Register Client => register a client to topic
 *          TUC command => SCDTMH:1.0\tTUC:<topic name>\n          // Topic Unregister Client => unregister a client from topic
 *          TSM command => SCDTMH:1.0\tTSM:<topic name>\n<message> // Topic Send Message => send a  message to topic
 *
 * @return 1 on success, 0 on failure
 *
 */
int SCDTopicServer::readHeader(QString message, Command &command)
{
   QTextStream ts(&message);

   QString head = ts.readLine(maxHeaderSize);

   if (head.isEmpty())
   {
      lastErrorMsg = "Null header line";
      return 0;
   }

   QStringList fields = head.split("\t",QString::SkipEmptyParts);

   if (fields.size()<2) // header must have at least two elements
   {
      lastErrorMsg = "Invalid header:" + head;
      return 0;
   }

   header.clear();

   if (checkHeaderField(fields.at(0),"SCDTMH")) // first item must be header type declaration
   {
      if (checkHeaderField(fields.at(1),command))
      {
         return head.length();
      }

      return head.length();
   }

   return 0;  // failure
}

/**
 * @brief SCDTopicServer::listLoadFromFile
 * @param fileName
 * @param list
 * @return
 */
int SCDTopicServer::listLoadFromFile(QString fileName, QStringList &list)
{
   list.clear();

   QFile f(fileName);

   if (!f.exists())
   {
      lastErrorMsg = "file '" + fileName + "' not found";
      return 0;
   }

   if (f.open(QIODevice::ReadOnly))
   {
      QByteArray buffer = f.readAll();

      if (buffer.size())
      {
         f.close();

         list << QString(buffer.constData()).split("\n",QString::SkipEmptyParts);

         return 1;
      }

      f.close();

      lastErrorMsg = "file '" + fileName + "' is empty => " + f.errorString();

      return -1;
   }

   lastErrorMsg = "error opening file '" + fileName + "' => " + f.errorString();

   return 0;
}

/**
 * @brief SCDTopicServer::listSaveToFile
 * @param fileName
 * @param list
 * @return
 */
int SCDTopicServer::listSaveToFile(QString fileName, QStringList &list, bool removeEmpty)
{
   if (list.isEmpty() && removeEmpty)
   {
      bool ret = QFile::remove(fileName);

      if (!ret)
      {
         lastErrorMsg = "error deleting empty file '" + fileName;
         return 0;
      }

      return 1;
   }

   QString topiclist = list.join("\n");

   QFile f(fileName);

   if (f.open(QIODevice::WriteOnly))
   {
      if (f.write(topiclist.toLatin1().constData())==-1)
      {
         f.close();
         lastErrorMsg = "error writing file '" + fileName + "' => " + f.errorString();
         return 0;
      }

      f.close();

      return 1;
   }

   lastErrorMsg = "error opening file '" + fileName + "' => " + f.errorString();

   return 0;
}

/**
 * @brief SignalsHandler::saveTopicList
 * @return
 */
int SCDTopicServer::saveTopicList()
{
   topics.removeDuplicates();

   return listSaveToFile("topics",topics);
}

/**
 * @brief SCDTopicServer::loadTopicList
 * @return
 */
int SCDTopicServer::loadTopicList()
{
   int ret = listLoadFromFile("topics",topics);

   if (ret>0)
   {
      topics.removeDuplicates(); // remove duplicates if any
   }

   return ret;
}

/**
 * @brief SCDTopicServer::topicExists
 * @param topic
 * @return
 */
bool SCDTopicServer::topicExists(QString topic)
{
   if (topic.trimmed().isEmpty())
   {
      return false;
   }

   QStringList filtered = topics.filter(topic);

   return !filtered.isEmpty();
}

/**
 * @brief SCDTopicServer::isDynamicTopic
 * @param topic
 * @return
 */
bool SCDTopicServer::isDynamicTopic(QString topic)
{
   bool dynamic = topics.contains(topic+":dynamic");

   return dynamic;
}

/**
 * @brief SCDTopicServer::getTopic get name of topic from topics string list
 * @param n topic index must be a valid index for topics string list
 * @return
 */
QString SCDTopicServer::getTopic(QStringList topics,int n)
{
   QString item = topics.at(n);

   QStringList items = item.split(":");

   return items.at(0);
}

/**
 * @brief SCDTopicServer::removeTopicClientList Remove the all topic subscribes: the file 'topic' will be removed, the subscribers list to topic
 * @param topic
 * @return 0: failure,
 *         1: success,
 *         2: success but warning: subscribers file not exists
 */
int SCDTopicServer::removeTopicSubscribers(QString topic)
{
   QFile f(topic);

   if (f.exists())
   {
      int ret = f.remove();

      if (!ret)
      {
         lastErrorMsg = "deleting subscribers file:" + topic +  " failed => " + f.errorString() ;
      }

      return ret;
   }

   lastErrorMsg = "file '" + topic + "' not found";

   return 2;
}

/**
 * @brief SignalsHandler::addTopic add new a topic to topic list: topic will be trimmed before inserting into topic list
 * @param topic
 * @return 0: failure,
 *         1: success,
 *         2: success, but warning: topic already exists
 */
int SCDTopicServer::addTopic(QString topic, bool dynamic)
{
   lastErrorMsg = "no error";

   if (!isValidTopicName(topic))
   {    
      return 0;
   }

   if (topicExists(topic))
   {
      lastErrorMsg = "topic '" + topic + "' already exists";

      return 2; // already exists
   }

   if (dynamic)
   {
      topic +=":dynamic";
   }
   else
   {
      topic +=":static";
   }

   topics.append(topic);

   return saveTopicList();
}

/**
 * @brief SignalsHandler::removeTopic
 * @param topic
 * @return  -1: failure, topic not exists
 *           0: failure,
 *           1: success,
 *           2: success, but warning: empty subscribers list
 */
int SCDTopicServer::removeTopic(QString topic, QStringList &removedSubscribers)
{
   lastErrorMsg = "no error";

   if (!isValidTopicName(topic))
   {
      return 0;
   }

   int index = topics.indexOf(QRegularExpression(topic + ":.*"));

   int retr = 1;

   if (index>-1)
   {
      topics.removeAt(index);
   }
   else
   {
      lastErrorMsg = "topic '" + topic + "' not found";
      return index;
   }

   if (QFile::exists(topic))
   {
      listLoadFromFile(topic,removedSubscribers); // load removed subscribers

      retr = removeTopicSubscribers(topic); // return 0 or 1 or 2 (warning empty list: subscribers file not exists)

      if (retr==0)
      {
         return retr; // remove topic subscriber failure
      }
   }

   int ret = saveTopicList(); // return 0 or 1

   if (ret==0)
   {
      return ret;
   }

   return 1; // return 1, 2 is ignored (subscribers file not found.)
}

/**
 * @brief SCDTopicServer::subscribeToTopic
 * @param topic
 * @param clientIp
 * @param createNewTopic
 * @return -3: subscription failed, but new topic has been created
 *         -2: subscription failed, but cannot create a new topic becose already exists
 *         -1: subscription failed  becose cannot create a new topic,
 *          0: topic subscribe failure,
 *          1: success,
 *          2: success, but warning: topic already exists
 *          3: success, new topic created
 */
int SCDTopicServer::subscribeToTopic(QString topic, QString clientIp, bool createNewTopic)
{
   lastErrorMsg = "no error";

   if (!isValidTopicName(topic))
   {
      return 0;
   }

   int newTopicRect = 1;

   if (createNewTopic)
   {
      newTopicRect = addTopic(topic,true); // 1: created, 2: already exists, 0: failure

      if (newTopicRect==0) // create dynamic topic
      {
         return -1;
      }
   }

   if (topicExists(topic)) // if topic exists
   {
      QStringList topicClients;

      listLoadFromFile(topic,topicClients); // try to load the topic subscribers list

      if (!topicClients.contains(clientIp)) // if is not subscribed to topic
      {
         topicClients.append(clientIp);     // subscribes to topic
      }

      int ret = listSaveToFile(topic,topicClients,true); // always save the subscribers list (this will rewrite corrupt or damaged file);

      if (createNewTopic)
      {
         switch (newTopicRect)
         {
            case 1: // new topic created.

              if (ret==0)
              {
                 return -3; // subscribe failed,
              }
              else
              {
                 return 3;  // subscribe success
              }

            break;

            case 2: // topic already exists

              if (ret==0)
              {
                 return -2; // subcribe failed
              }
              else
              {
                 return 2;  // subscribe success
              }

            break;
         }
      }

      return ret; // 0: subscribe failure, 1: subscribe success
   }

   lastErrorMsg = "topic '" + topic + "' not found";

   return 0;
}

/**
 * @brief SCDTopicServer::unscribeFromTopic unscribe client from topic
 * @param topic
 * @param clientIp
 * @return   0: unscribe failed.
 *           1: unscribe ok
 *           2: succes but warning: client already unscribed becose topic not exists
 *           3: unscribe ok, topic removed
 *           4: success but warning: delete empty dynamic topic failure
 */
int SCDTopicServer::unscribeFromTopic(QString topic, QString clientIp)
{
   lastErrorMsg = "no error";

   if (!isValidTopicName(topic))
   {
      return 0;
   }

   if (topicExists(topic)) // if is a registered topic
   {
      QStringList subscribers;

      int ret = 1;

      //****************************************************************************************************************************
      //
      // This section remove the client from list of subscribers to topic.
      //
      // return 0 on failure, otherwise continue below to remove topic if needed and return
      //

      if (QFile::exists(topic)) // if the list of subscribes to topic exists
      {
         ret = listLoadFromFile(topic, subscribers); // try to load the list of subscribers to topic

         if (ret<=0) // assumes the file 'topic' to be corrupted and try to remove it
         {
            ret = removeTopicSubscribers(topic); // remove the list of subscribers to topic if damaged.

            if (ret==0)
            {
               return 0; // remove topic subscribers list failure
            }
         }
         else
         {
            if (subscribers.removeAll(clientIp)) // remove the client from list of subscribers to topic if any
            {
               ret = listSaveToFile(topic, subscribers, true); // save the list of subscribers to topic or delete it if empty

               if (!ret)
               {
                  return ret;
               }
            }
         }         
      }

      //*************************************************************************************************************************
      // this section remove  the topic only if subscribers list is empty. At this stage if subscribers list is empty means that
      // the  topic subscribers file not exists (is already deleted): cannot never return 2

      if (subscribers.isEmpty() && isDynamicTopic(topic))
      {
         QStringList subscribers;

         return (removeTopic(topic,subscribers) == 0) ? -1 : 3; // translate error code 0 to -1, return -1 or 3 success (topic delete)
      }

      return ret; // return 0: unscribe from topic failure, 1: unscribe success (client is unscribed, becose subscribers file not exists)
   }
   else
   {
      removeTopicSubscribers(topic); // remove orphan client list (if exists)
   }

   lastErrorMsg = "topic '" + topic + "' not found";

   return 2;
}

/**
 * @brief SCDTopicServer::unscribeFromTopics unscribe the client from all topics, the resulting dynamic topic having
 *                                           an empty subscribers list will be removed
 * @param clientIp
 * @param removeEmptyTopic
 * @return error list
 */
QStringList SCDTopicServer::unscribeFromTopics(QString clientIp)
{
   QString     topic;
   QStringList errors;
   QStringList topics = getTopics();

   int ret;

   for (int n=0; n<topics.length(); n++)  // iterate the topics
   {
      topic = getTopic(topics,n);

      ret = unscribeFromTopic(topic, clientIp); // remove client subscription to topic

      if (ret==0)
      {
         errors << lastErrorMsg;
      }
   }

   return errors;
}

/**
 * @brief SCDTopicServer::unscribeAllSubcribers remove all subscribers subscriptions to each topics: topics list must be already loaded before;
 *                                              dynamic topic wil be removed
 */
void SCDTopicServer::unregisterAllSubscriptions()
{
   QString topic;

   QStringList topics = getTopics();

   QStringList subscribers;

   for (int n=0; n<topics.size(); n++)
   {
      topic = getTopic(topics,n);

      subscribers.clear();

      if (isDynamicTopic(topic))
      {         
         removeTopic(topic, subscribers); // remove topic and its subscribers
      }
      else
      {
         listLoadFromFile(topic,subscribers);

         removeTopicSubscribers(topic);
      }
   }
}

/**
 * @brief SCDTopicServer::notifyToSubscribers Send a message to the subscribers list, except the sender
 *                                            Subscribers list should be loaeded from file of topic subscribers
 * @param subscribers a topic subscribers list
 * @param notifyMsg
 * @return
 */
int SCDTopicServer::sendMessageToSubscribers(QStringList subscribers, QString notifyMsg, QString sender)
{
   QString subscriber;

   QWebSocket *socket;

   for (int n=0; n<subscribers.size(); n++)
   {
      subscriber = subscribers.at(n);

      if (subscriber==sender)
      {
         continue;
      }

      if (sockList.contains(subscriber))
      {
         socket = sockList[subscriber];

         if (socket->isValid())
         {
            socket->sendTextMessage(notifyMsg);
         }
      }
   }

   return 1;
}

/**
 * @brief SCDTopicServer::isValidTopicName remove trailng space from topic name and check if empty
 * @param topic
 * @return true if is an not empty topic name.
 */
bool SCDTopicServer::isValidTopicName(QString &topic)
{
   topic = topic.trimmed();

   if (topic.isEmpty())
   {
       lastErrorMsg = "Undefined topic name";
       return false;
   }

   return true;
}

/**
 * @brief SCDTopicServer::addressToString
 * @param address
 * @param peer
 * @return
 */
QString SCDTopicServer::addressToString(QHostAddress address, int port)
{
   return address.toString().toUpper() + ":" + QString::number(port).toUpper();
}

/**
 * @brief SCDTopicServer::addressToHex
 * @param address
 * @param peer
 * @return
 */
QString SCDTopicServer::addressToHex(QHostAddress address, int port)
{
   return QString::number(address.toIPv4Address(),16).toUpper() + ":" + QString::number(port,16).toUpper();
}

/**
 * @brief SCDTopicServer::lastError
 * @return
 */
QString SCDTopicServer::lastError()
{
   return lastErrorMsg;
}

/**
 * @brief SCDTopicServer::getTopics
 * @return
 */
QStringList SCDTopicServer::getTopics()
{
   return topics;
}

/**
 * @brief SCDTopicServer::sendMessageToTopic send message to all topic subscribers except to sender.
 *                                           the client can send a message to the topics to which he is not subscribed
 * @param topic
 * @param message
 * @return o if topic not exists, 1 otherwise
 */
int SCDTopicServer::sendMessageToTopic(QString topic, QString message, QString sender)
{
   lastErrorMsg = "no error";

   if (!topicExists(topic))
   {
      lastErrorMsg = "topic '" + topic + "' not found";

      return 0;
   }

   QStringList subscribers;

   listLoadFromFile(topic,subscribers);

   QString subscriber;

   QWebSocket *socket;

   for (int n=0; n<subscribers.size(); n++)
   {
      subscriber = subscribers.at(n);

      if (subscriber==sender)
      {
         continue;
      }

      if (sockList.contains(subscriber))
      {
         socket = sockList[subscriber];

         if (socket->isValid())
         {
            socket->sendTextMessage("[" + sender + "@" + topic +"]:" + message);
         }
      }
   }

   return 1;
}
