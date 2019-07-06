#ifndef SCDTOPICCLIENT_H
#define SCDTOPICCLIENT_H

#include <QObject>
#include <QByteArray>
#include <QWebSocket>
#include <QDir>
/**
 * @brief The SCDTopicClient class
 */
class SCDTopicClient : public QWebSocket
{
  Q_OBJECT

  private:

    QString host;
    quint16 port;

    QString lastError;
    QString message;

    bool registered;

    void emitNotifySignal(QString message, QString topic, int statusCode, QString errMsg);

  public:

    enum StatusCode{SC_ERROR=0,SC_SUCCESS=1,SC_WARNING=2}; // SC_WARNING should be treated as SC_SUCCESS

    SCDTopicClient();

    ~SCDTopicClient();

  public slots:

    int  connectToHost(QString host, quint16 port);

    int sendMessageToTopic(QString msg, QString topic);
    int registerToTopic(QString topic, bool createNewTopic=true);
    int unregisterToTopic(QString topic);
    int makeTopic(QString topic);
    int deleteTopic(QString topic);

    int getAllTopics();

  private slots:

    void onTextMessageReceived(const QString &message);
    // void onBinaryMessageReceived(const QByteArray &message);

  signals:

    void topicMessageReceived(QString topic, QString message);    
    void notifyMessage(QString message, QString topic, int statusCode, QString errMess);

    void notifyNewTopic(QString topic, int statusCode, QString errMsg);
    void notifyDeleteTopic(QString topic, int statusCode, QString errMsg);
    void notifyTopicSubscription(QString topic, int statusCode, QString errMsg);
    void notifyTopicUnscribe(QString topic, int statusCode, QString errMsg);
    void notifyMessageSent(QString topic, int statusCode, QString errMsg);
};

#endif // SCDTOPICCLIENT_H
