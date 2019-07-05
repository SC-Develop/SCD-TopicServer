#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
   ui->setupUi(this);

   connect(&tc,SIGNAL(connected()),this,SLOT(onConnected()));
   connect(&tc,SIGNAL(disconnected()),this,SLOT(onDisconected()));   
   connect(&tc,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(onError(QAbstractSocket::SocketError)));

   connect(&tc,SIGNAL(topicMessageReceived(QString,QString)),this,SLOT(onTopicMessageReceived(QString,QString)));

   connect(&tc,SIGNAL(notifyMessage(QString,QString,int,QString)),this,SLOT(onNotifyMessage(QString,QString,int,QString)));

   connect(&tc,SIGNAL(notifyNewTopic(QString,int,QString)),this,SLOT(onNotifyNewTopic(QString,int,QString)));
   connect(&tc,SIGNAL(notifyDeleteTopic(QString,int,QString)),this,SLOT(onNotifyDeleteTopic(QString,int,QString)));

   connect(&tc,SIGNAL(notifyTopicSubscription(QString,int,QString)),this,SLOT(onNotifyTopicSubscription(QString,int,QString)));
   connect(&tc,SIGNAL(notifyTopicUnscribe(QString,int,QString)),this,SLOT(onNotifyTopicUnscribe(QString,int,QString)));

   connect(&tc,SIGNAL(notifyMessageSent(QString,int,QString)),this,SLOT(onNotifyMessageSent(QString,int,QString)));

   enableControls(false);
}

/**
 * @brief MainWindow::~MainWindow
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief MainWindow::oncConnected
 */
void MainWindow::onConnected()
{
   ui->groupBox_server->setEnabled(true);

   ui->toolButton_connect->setText("Close");
   ui->toolButton_connect->setToolTip("Close connection");

   QPixmap pixmap;

   pixmap.load(":/images/green_circle.png");

   ui->label_connected->setPixmap(pixmap);

   enableControls(true);
}

/**
 * @brief MainWindow::onDisconeccted
 */
void MainWindow::onDisconected()
{
   ui->toolButton_connect->setText("Connect");
   ui->toolButton_connect->setToolTip("Connect to server");

   QPixmap pixmap;

   pixmap.load(":/images/red_circle.png");

   ui->label_connected->setPixmap(pixmap);   

   enableControls(false);

   // ui->toolButton_connect->setEnabled(true);
}

/**
 * @brief MainWindow::onError
 */
void MainWindow::onError(QAbstractSocket::SocketError error)
{
   if (tc.state()==QAbstractSocket::UnconnectedState)
   {
      onDisconected();
   }
   else
   {
      tc.abort();
   }
}

/**
 * @brief MainWindow::onTopicMessageReceived
 * @param topic
 * @param message
 */
void MainWindow::onTopicMessageReceived(QString topic, QString message)
{
   ui->textBrowser_topics->setText(ui->textBrowser_topics->toPlainText() + topic + ": " + message + "\n");
}

/**
 * @brief MainWindow::onNotifyMessage
 * @param message
 * @param topic
 * @param error
 */
void MainWindow::onNotifyMessage(QString message, QString topic, int error, QString errMess)
{

}

/**
 * @brief MainWindow::onNotifyNewTopic
 * @param topic
 * @param statusCode
 * @param errMsg
 */
void MainWindow::onNotifyNewTopic(QString topic, int statusCode, QString errMsg)
{
   QString response;

   switch (statusCode)
   {
      case SCDTopicClient::SC_ERROR:

        response = "add new topic '" + topic + "' failed\n[error] " + errMsg + "\n";

      break;

      case  SCDTopicClient::SC_SUCCESS:

        response = "added new topic '" + topic + "'\n";

        ui->comboBox->addItem(topic);

      break;

      case  SCDTopicClient::SC_WARNING:

        response = "add new topic '" + topic + "'\n[warning] " + errMsg + "\n";

        ui->comboBox->addItem(topic);

      break;

      default:

        response = "creating new topic '" + topic + "' => undefined status code: " + QString::number(statusCode) + "\n";

      break;
   }

   ui->textBrowser_topics->setText(ui->textBrowser_topics->toPlainText() + "[notify] " + response);
}

/**
 * @brief MainWindow::onNotifyDeleteTopic
 * @param topic
 * @param statusCode
 * @param errMsg
 */
void MainWindow::onNotifyDeleteTopic(QString topic, int statusCode, QString errMsg)
{
   QString response;

   switch(statusCode)
   {
      case SCDTopicClient::SC_ERROR:

         response = "deleting topic '" + topic + "' failed\n[error] " + errMsg + "\n";

      break;

      case SCDTopicClient::SC_SUCCESS:
      {
         response = "topic '" + topic + "' deleted\n";

         int index = ui->comboBox->findText(topic);

         ui->comboBox->removeItem(index);
      }
      break;

      case SCDTopicClient::SC_WARNING:
      {
         response = "deleting topic '" + topic + "'\n[warning] " + errMsg + "\n";

         int index = ui->comboBox->findText(topic);

         ui->comboBox->removeItem(index);
      }
      break;
   }

   ui->textBrowser_topics->setText(ui->textBrowser_topics->toPlainText() + "[notify] " + response);
}

/**
 * @brief MainWindow::onNotifyTopicSubscription
 * @param topic
 * @param statusCode
 * @param errMsg
 */
void MainWindow::onNotifyTopicSubscription(QString topic, int statusCode, QString errMsg)
{
   QString response;

   if (statusCode==SCDTopicClient::SC_ERROR)
   {
      response.append("subscription to the topic '" + topic + "' failed\n[error ] " + errMsg + "\n");
   }
   else
   {
      response.append("subscribed to topic '" + topic + "'\n");

      if (ui->comboBox->findText(topic)==-1)
      {
         ui->comboBox->addItem(topic);
      }
   }

   ui->textBrowser_topics->setText(ui->textBrowser_topics->toPlainText() + "[notify] " + response);
}

/**
 * @brief MainWindow::OnNotifyTopicUnscribe
 * @param topic
 * @param statusCode
 * @param errMsg
 */
void MainWindow::onNotifyTopicUnscribe(QString topic, int statusCode, QString errMsg)
{
   QString response;

   switch (statusCode)
   {
      case SCDTopicClient::SC_ERROR:

        response = "unscribing from topic '" + topic + "' failed\n[error] " + errMsg + "\n";

      break;

      case  SCDTopicClient::SC_SUCCESS:
      {
         response = "unscribed from topic '" + topic + "'\n";

         int index = ui->comboBox->findText(topic);

         ui->comboBox->removeItem(index);
      }
      break;

      case  SCDTopicClient::SC_WARNING:
      {
         response = "unscribing from topic '" + topic + "'\n[warning] " + errMsg + "\n";

         int index = ui->comboBox->findText(topic);

         ui->comboBox->removeItem(index);
      }
      break;

      default:

        response = "unscribing from topic '" + topic + "' => undefined status code: " + QString::number(statusCode) + "\n";

      break;
   }

   ui->textBrowser_topics->setText(ui->textBrowser_topics->toPlainText() + "[notify] " + response);
}

/**
 * @brief MainWindow::onMessageSent
 * @param topic
 * @param statusCode
 * @param errMsg
 */
void MainWindow::onNotifyMessageSent(QString topic, int statusCode, QString errMsg)
{
   QString response;

   if (statusCode==SCDTopicClient::SC_ERROR)
   {
      response.append("send message to topic '" + topic + "'\n[error ] " + errMsg +"\n");
   }
   else
   {
      response.append("message sent to topic '" + topic + "'\n");
   }

   ui->textBrowser_topics->setText(ui->textBrowser_topics->toPlainText() + "[notify] " + response);
}

/**
 * @brief MainWindow::on_toolButton_connect_clicked
 */
void MainWindow::on_toolButton_connect_clicked()
{
   if (tc.state()==QAbstractSocket::UnconnectedState)
   {
      tc.connectToHost(ui->lineEdit_ip->text(),ui->lineEdit_port->text().toInt());

      ui->groupBox_server->setEnabled(false);

      ui->textBrowser_topics->clear();

      ui->comboBox->clear();
   }
   else
   {
      tc.close();
      // ui->toolButton_connect->setEnabled(false);
   }
}

/**
 * @brief MainWindow::on_toolButton_2_clicked
 */
void MainWindow::on_toolButton_2_clicked()
{
   if (!ui->comboBox->currentText().isEmpty())
   {
      tc.sendMessageToTopic(ui->lineEdit_msg->text(),ui->comboBox->currentText());
   }
}

/**
 * @brief MainWindow::on_toolButton_clicked
 */
void MainWindow::on_toolButton_clicked()
{
   QString topic = ui->lineEdit_topic->text();

   int index = ui->comboBox->findText(topic);

   if (index==-1)
   {
      bool checkbox = ui->checkBox->isChecked();

      tc.registerToTopic(topic,checkbox);
   }
   else
   {
      ui->comboBox->setCurrentIndex(index);
   }
}

/**
 * @brief MainWindow::on_toolButton_unscribe_clicked
 */
void MainWindow::on_toolButton_unscribe_clicked()
{
   tc.unregisterToTopic(ui->comboBox->currentText());
}

/**
 * @brief MainWindow::on_toolButton_topicDelete_clicked
 */
void MainWindow::on_toolButton_topicDelete_clicked()
{
    tc.deleteTopic(ui->comboBox->currentText());
}

/**
 * @brief MainWindow::enableControls
 * @param enabled
 */
void MainWindow::enableControls(bool enabled)
{
   ui->frame_topic->setEnabled(enabled);
   ui->groupBox_topics->setEnabled(enabled);
}
