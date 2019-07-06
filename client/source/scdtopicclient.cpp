/**
 * @class SCDTopicClient
 *
 * @brief SCD Topic Client allow you to connect to SCD Topic Server and create/subscribe topics
 *
 *        SCD Topic Sever is a Web Socket Server which allow you to notify events to all client processes
 *        subscribed to the same topic.
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com
 *
 * @copyright (c) 2019 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 *
*/

#include <QDebug>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QUrl>
#include <QDateTime>

#include "scdtopicclient.h"

/**
 * @brief SCDTopicClient::SCDTopicClient
 */
SCDTopicClient::SCDTopicClient() : QWebSocket()
{
   connect(this,SIGNAL(textMessageReceived(QString)),this,SLOT(onTextMessageReceived(QString)));
}

/**
 * @brief SCDTopicClient::~SCDTopicClient
 */
SCDTopicClient::~SCDTopicClient()
{
   qDebug() << "Destroy web socket Topic Client";
}

/**
 * @brief SCDTopicClient::QWebSocket::~QWebSocket
 */
SCDTopicClient::QWebSocket::~QWebSocket()
{
   qDebug() << "Destroy web socket";
}

/**
 * @brief SCDTopicClient::_connected executed on client connection signal
 */
int SCDTopicClient::connectToHost(QString host, quint16 port)
{
   QUrl url=QUrl(QString("ws://"+host+":"+QString::number(port)));

   open(url);

   return 1;
}

/**
 * @brief SCDTopicClient::sendMessage
 * @param message
 * @return
 */
int SCDTopicClient::sendMessageToTopic(QString msg, QString topic)
{
   if (isValid())
   {
      message = "SCDTMH:1.0\tTSM:" + topic + "\n" + msg;

      return sendTextMessage(message);
   }

   lastError = "Can't read or write socket";

   return 0;
}

/**
 * @brief SCDTopicClient::registerToTopic
 * @param topic
 * @param createNewTopic
 */
int SCDTopicClient::registerToTopic(QString topic, bool createNewTopic)
{
   if (isValid())
   {
      if (createNewTopic)
      {
         message =  "SCDTMH:1.0\tTRN:" + topic + "\n";
      }
      else
      {
         message =  "SCDTMH:1.0\tTRC:" + topic + "\n";
      }

      return sendTextMessage(message);
   }   

   lastError = "Can't read or write socket";

   return 0;
}

/**
 * @brief SCDTopicClient::unregisterToTopic
 * @param topic
 * @param removeEmptyTopic
 */
int SCDTopicClient::unregisterToTopic(QString topic)
{
   if (isValid())
   {
      message =  "SCDTMH:1.0\tTUC:" + topic + "\n";

      return sendTextMessage(message);
   }

   lastError = "Can't read or write socket";

   return 0;
}

/**
 * @brief SCDTopicClient::makeTopic
 * @param topic
 */
int SCDTopicClient::makeTopic(QString topic)
{
   if (isValid())
   {
      message =  "SCDTMH:1.0\tTMK:" + topic + "\n";

      return sendTextMessage(message);
   }

   lastError = "Can't read or write socket";

   return 0;
}

/**
 * @brief SCDTopicClient::deleteTopic
 * @param topic
 */
int SCDTopicClient::deleteTopic(QString topic)
{
   if (isValid())
   {
      message =  "SCDTMH:1.0\tTDL:" + topic + "\n";

      return sendTextMessage(message);
   }

   lastError = "Can't read or write socket";

   return 0;
}

/**
 * @brief SCDTopicClient::getAllTopics
 * @return
 */
int SCDTopicClient::getAllTopics()
{
  return 0;
}

/**
 * @brief SCDTopicClient::onTextMessageReceived
 * @param message
 */
void SCDTopicClient::onTextMessageReceived(const QString &message)
{
   if (message[0]=="[")
   {
     int pos = message.indexOf("]:"); // find the end of message header

     QString header = message.mid(1,pos-1);

     QStringList items = header.split("@");

     if (items.size()==2)
     {
        QString sender = items[0];
        QString topic  = items[1];
        QString mess   = message.mid(pos+2);

        if (sender=="server" && topic=="notify")
        {
           QStringList items = mess.split("|");

           QString errMess;
           QString topic;
           QString message;

           int statusCode;

           if (items.size()>2)
           {
              message    = items[0].trimmed();
              topic      = items[1].trimmed();
              statusCode = items[2].trimmed().toInt();

              if (items.size()>3)
              {
                 errMess = items[3].trimmed();
              }
           }
           else
           {
              statusCode = SC_ERROR;
              errMess    = "Invalid notify format";
           }

           emitNotifySignal(message, topic, statusCode, errMess);
        }
        else
        {
           emit topicMessageReceived(topic, mess);
        }
     }
     else
     {
        emit notifyMessage(message, "", SC_ERROR, "Invalid message");
     }
   }
}

/**
 * @brief SCDTopicClient::SCDTopicClient
 * @param Host
 * @param Port
 * @param Timeout
 */
void SCDTopicClient::emitNotifySignal(QString message, QString topic, int statusCode, QString errMsg)
{
   emit notifyMessage(message, topic, statusCode, errMsg);

   if (message=="TMK") // unused for this application
   {
      emit notifyNewTopic(topic,statusCode,errMsg);
   }
   else
   if (message == "TRN")
   {
      if (statusCode<0 || statusCode>1)
      {
         emit notifyNewTopic(topic, (statusCode==-1) ? SC_ERROR : (abs(statusCode)==2) ? SC_WARNING : SC_SUCCESS, errMsg);
      }

      emit notifyTopicSubscription(topic, (statusCode <=0) ? SC_ERROR : statusCode, errMsg);
   }
   else
   if (message == "TRC")
   {
      emit notifyTopicSubscription(topic, statusCode, errMsg);
   }
   else
   if (message == "TUC")
   {
      emit notifyTopicUnscribe(topic, (statusCode==3) ? SC_SUCCESS : statusCode, errMsg);

      if (statusCode>2)
      {
         emit notifyDeleteTopic(topic, (statusCode == 4) ? SC_ERROR : SC_SUCCESS, errMsg);
      }
   }
   else
   if (message == "TDL")
   {
      emit notifyDeleteTopic(topic, (statusCode<=0) ? SC_ERROR : (statusCode==1) ? SC_SUCCESS : SC_WARNING, errMsg);
   }
   else
   if (message == "TSM")
   {
      emit notifyMessageSent(topic, statusCode, errMsg);
   }
}
