#ifndef SCDTOPICSERVER_H
#define SCDTOPICSERVER_H

#include <QList>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QHostAddress>
#include <QByteArray>

class SCDTopicServer : public QWebSocketServer
{
   Q_OBJECT

   private:

     enum Command {TMK=0,TDL=1,TRC=2,TUC=3,TRN=4,TSM=5};

     int command;

     QStringList commands;

     int port;

     QString lastErrorMsg;

     QHash <QString, QWebSocket *> sockList;

     QStringList topics;

     QMap <QString,QVariant> header; // current header entries readed

     int maxHeaderSize;

     QMap <QString,QVariant> getHeader();

     QString addressToHex(QWebSocket *socket);
     QString addressToString(QWebSocket *socket);

     int checkHeaderField(const QString &headerItem, const QString &fieldName);

     int checkHeaderField(const QString &headerItem, enum Command &cmd);

     int readHeader(QString message, Command &command);

     int listLoadFromFile(QString fileName, QStringList &list);
     int listSaveToFile(QString fileName, QStringList &list, bool removeEmpty=false);

     int saveTopicList();
     int loadTopicList();

     QString getTopic(QStringList topics, int n);

     int subscribeToTopic(QString topic, QString clientIp, bool createNewTopic=true);
     int unscribeFromTopic(QString topic, QString clientIp);

     QStringList unscribeFromTopics(QString clientIp);

     int removeTopicSubscribers(QString topic);

     int sendMessageToTopic(QString topic, QString message, QString sender);

     void unregisterAllSubscriptions();

     int sendMessageToSubscribers(QStringList subscribers, QString notifyMsg, QString sender);

     bool isValidTopicName(QString &topic);

   public:

     explicit SCDTopicServer(QObject *parent = 0, int port=12345);

     ~SCDTopicServer();

     int start(); // Start tcp server for incoming connections
     void stop(); // Start tcp server for incoming connections

     QString lastError();
     QStringList getTopics();

     bool topicExists(QString topic);
     bool isDynamicTopic(QString topic);

     int addTopic(QString topic, bool dynamic=false);
     int removeTopic(QString topic, QStringList &removedSubscribers);

     QString addressToString(QHostAddress address, int port);
     QString addressToHex(QHostAddress address, int port);

   signals:

   private slots:

     void onNewConnection();
     void onDisconnected();
     void onTextMessageReceived(QString message);
     void onSocketError(QAbstractSocket::SocketError error);
     void onAboutToClose();
     //void onBinaryMessageReceived(QByteArray message);
};

#endif // SCDTOPICSERVER_H
