#include "login.h"
#include "ui_login.h"
#include <QPainter>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QNetworkRequest>      // 请求
#include <QNetworkReply>        // 响应
#include <QDebug>
#include "common/logininfoinstance.h"
#include "common/common.h"
#pragma execution_character_set("utf-8")

#define DEBUG

Login::Login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);

    // 给title_wg窗口设置父窗口类对象
    ui->title_wg->setParent(this);

    //========== 初始化对象=========
    // 创建mainWindows对象
    m_mainwin = new MainWindow;

    // 初始化网络
    m_manager = Common::getNetManager();

    // 加密对象
    m_aeskey = AesKeyInstance::getSpyObj();

    // ==========登录窗口设置============
    // 去除边框
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());

    // 设置当前窗口的所有字体
    // this->setFont(QFont("新宋体", 12, QFont::Bold, false));

    // password修改成保密符号
    ui->sginin_pwd->setEchoMode(QLineEdit::Password);
    ui->resgin_pwd->setEchoMode(QLineEdit::Password);
    ui->resgin_pwd_con->setEchoMode(QLineEdit::Password);

    // 窗口图标设置
    this->setWindowIcon(QIcon(":/image/logo.ico"));
    m_mainwin->setWindowIcon(QIcon(":/image/logo.ico"));

    // 默认开启软件显示的窗口
    ui->stackedWidget->setCurrentWidget(ui->sginin_page);

//    // 设置输入焦点
//    ui->sginin_usr->setFocus();

    // =======titlewidget信号处理=======
    // 处理关闭页面
    connect(ui->title_wg,&TitleWidget::closeWindow,[=]()
    {
        this->close();
    });

    // 处理转换页面
    connect(ui->title_wg,&TitleWidget::showSetWg,[=]()
    {
        // 页面跳转至设置界面
        ui->stackedWidget->setCurrentWidget(ui->serverset_page);
        // 设置输入焦点
        ui->server_ip->setFocus();
    });

    // ======== 登录页面======
    // 注册按钮跳转
    connect(ui->resgin_btn,&QPushButton::clicked,this,[=]()
    {
        ui->stackedWidget->setCurrentWidget(ui->resgin_page);
        // 设置输入焦点
        ui->resgin_usr->setFocus();
    });
    // ======== 注册页面=======
    connect(ui->resgin_canel_btn,&QPushButton::clicked,this,[=](){

        ui->stackedWidget->setCurrentWidget(ui->sginin_page);
        // 设置输入焦点
        ui->sginin_usr->setFocus();

        // 取消则清空注册页面的数据
        ui->resgin_usr->clear();
        ui->resgin_nickname->clear();
        ui->resgin_pwd->clear();
        ui->resgin_pwd_con->clear();
        ui->resgin_email->clear();
        ui->resgin_phone->clear();
    });
    // ======== 设置页面=======
    connect(ui->ser_cancel_btn,&QPushButton::clicked,this,[=](){

        ui->stackedWidget->setCurrentWidget(ui->sginin_page);
        // 设置输入焦点
        ui->sginin_usr->setFocus();
    });

    // =========主页面跳转登录=====
    connect(m_mainwin,&MainWindow::changeUser,[=]()
    {
        // 隐藏操作界面
        m_mainwin->hide();
        // 显示登录界面
        this->show();
        // 将界面居中
        m_common.moveToCenter(this);
    });
    // 读取配置信息
    readCfg();

    // 加载图片信息 - 显示文件列表的时候使用，在此初始化
    m_common.getFileTypeList();
}

Login::~Login()
{
    delete ui;
}

QByteArray Login::SetSgininJson(QString user, QString pwd)
{
    QMap<QString,QVariant> loginArray;
    loginArray.insert("user",user);
    loginArray.insert("password",pwd);
    // 将键值队转化为json格式
    QJsonDocument jsonDocument = QJsonDocument::fromVariant(loginArray);
    if(jsonDocument.isNull())
    {
         cout << " jsonDocument is NULL ";
         return "";
    }
    return jsonDocument.toJson();
}

QByteArray Login::SetResginJson(QString user, QString nickname, QString pwd, QString phone, QString email)
{
    QMap<QString,QVariant> resginArray;
    resginArray.insert("user_name",user);
    resginArray.insert("nick_name",nickname);
    resginArray.insert("password",pwd);
    resginArray.insert("phone",phone);
    resginArray.insert("email",email);

    QJsonDocument jsonDpcument = QJsonDocument::fromVariant(resginArray);
    if(jsonDpcument.isNull())
    {
        cout << " jsonDocument is NULL ";
        return "";
    }
    return jsonDpcument.toJson();
}

QStringList Login::getSgininStatus(QByteArray json)
{
    QJsonParseError error;
    QStringList list;

    QJsonDocument doc = QJsonDocument::fromJson(json,&error);
    if(error.error == QJsonParseError::NoError)
    {
        if(doc.isNull() || doc.isEmpty())
        {
            cout << "doc.isNull() || doc.isEmpty()";
            return list;
        }
        if(doc.isObject())
        {
            QJsonObject obj = doc.object();
            // 状态码
            list.push_back(obj.value("retcode").toString());
            // 登录的token
            list.push_back(obj.value("token").toString());
        }
    }
    else
    {
        cout << error.errorString();
    }
    return list;
}

void Login::on_sginin_btn_clicked()
{
    // 获取用消息
    QString user = ui->sginin_usr->text();
    QString pwd = ui->sginin_pwd->text();
    QString serverip = ui->server_ip->text();
    QString serverport = ui->server_port->text();

    // 验证数据
    QRegExp regexp(USER_REG);
    if(!regexp.exactMatch(user))
    {
        // 弹窗显示
//        QMessageBox::warning(this,"警告","用户名名格式不正确");
        ui->sginin_status->setText("用户名名格式不正确");
        ui->sginin_usr->clear();
        ui->sginin_usr->setFocus();
        return;
    }
    regexp.setPattern(PASSWD_REG);
    if(!regexp.exactMatch(pwd))
    {
//        QMessageBox::warning(this,"警告","密码格式不正确");
        ui->sginin_status->setText("密码格式不正确");
        ui->sginin_pwd->clear();
        ui->sginin_pwd->setFocus();
        return;
    }
    // 清除状态提示
    ui->sginin_status->clear();

    // 登录信息加密，判断是否要保存
    m_common.writeSgininInfo(user,pwd,ui->rember_pwd->isChecked());
    // 将数据打包成json格式
    QByteArray array = SetSgininJson(user,pwd);
    cout << "sginin json data "<< array;

    //
    QNetworkRequest request;
    // 设置请求头
    QString url = QString("http://%1:%2/login").arg(serverip).arg(serverport);
    request.setUrl(QUrl(url));
    // 设置请求行
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/json"));
    request.setHeader(QNetworkRequest::ContentLengthHeader,QVariant(array.size()));

    QNetworkReply *reply = m_manager->post(request,array);

    // 接收服务器发送回来的相应·
#ifdef DEBUG
    // 隐藏当前窗口
    this->hide();
    // 跳转到主界面上
    m_mainwin->showMainWindos();


    LoginInfoInstance *ptr = LoginInfoInstance::getInstance();
    ptr->setLoginInfo(user,serverip,serverport,"sdasdasdasd");

#else
    connect(reply,&QNetworkReply::finished,[=]()
    {
        // 出错
        if(reply->error() != QNetworkReply::NoError)
        {
            cout << reply->errorString();
            return ;
        }

        // 读取返回的数据
        QByteArray json = reply->readAll();
        QStringList code = getSgininStatus(json);
        if(code.at(0) == "000")
        {
            cout << "登录成功！";

            //
            LoginInfoInstance *ptr = LoginInfoInstance::getInstance();
            ptr->setLoginInfo(user,serverip,serverport,code.at(1));
            cout << ptr->getUser().toUtf8().data() << "," << ptr->getIp() << "," << ptr->getPort() << code.at(1);
            // 隐藏当前窗口
            this->hide();
            // 跳转到主界面上
            m_mainwin->showMainWindos();
        }
        else
        {
            ui->sginin_status->setText("登录失败，用户名或密码错误！");
        }
        reply->deleteLater();
    });
#endif
}

void Login::on_resgin_confirm_btn_clicked()
{
    // 从控件中取出用户输入的数据
    QString user = ui->resgin_usr->text();
    QString nickname = ui->resgin_nickname->text();
    QString pwd = ui->resgin_pwd->text();
    QString pwdcon = ui->resgin_pwd_con->text();
    QString phone = ui->resgin_phone->text();
    QString email = ui->resgin_email->text();

    // 验证数据
    QRegExp regexp(USER_REG);
    if(!regexp.exactMatch(user))
    {
        ui->re_status->setText("用户名格式不正确");
        ui->resgin_usr->clear();
        ui->resgin_usr->setFocus();
        return;
    }
    if(!regexp.exactMatch(nickname))
    {
        ui->re_status->setText("昵称格式不正确");
        ui->resgin_nickname->clear();
        ui->resgin_nickname->setFocus();
        return;
    }
    regexp.setPattern(PASSWD_REG);
    if(!regexp.exactMatch(pwd))
    {
        ui->re_status->setText("密码格式不正确");
        ui->resgin_pwd->clear();
        ui->resgin_pwd->setFocus();
        return;
    }
    if(pwd != pwdcon)
    {
        ui->re_status->setText("两次密码不匹配，请重新输入");
        ui->resgin_pwd_con->clear();
        ui->resgin_pwd_con->setFocus();
        return;
    }
    regexp.setPattern(PHONE_REG);
    if(!regexp.exactMatch(phone))
    {
        ui->re_status->setText("电话号码格式不正确");
        ui->resgin_phone->clear();
        ui->resgin_phone->setFocus();
        return;
    }
    regexp.setPattern(EMAIL_REG);
    if(!regexp.exactMatch(email))
    {
        ui->re_status->setText("邮箱格式不正确");
        ui->resgin_email->clear();
        ui->resgin_email->setFocus();
        return;
    }
    ui->re_status->clear();

    // 将数据打包成json格式
    QByteArray array = SetResginJson(user,nickname,m_common.getStrSha256(pwd),phone,email);
    qDebug() << "resgin json data "<< array;

    QNetworkRequest request;
    // 请求行
    QString url = QString("http://%1:%2/reup").arg(ui->server_ip->text()).arg(ui->server_port->text());
    request.setUrl(QUrl(url));
    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/json"));     // 请求类型
    request.setHeader(QNetworkRequest::ContentLengthHeader,QVariant(array.size()));          // 数据大小

    //发送请求
    QNetworkReply* reply = m_manager->post(request,array);


    // 获取响应信号
    connect(reply,&QNetworkReply::readyRead,[=]()
    {
        QByteArray jsonDate = reply->readAll();
        // 读取retcode状态

        QString code = m_common.getCode(jsonDate);
        if(code == "010")
        {
            ui->re_status->setText("注册成功！正在为您返回登录界面..");

            // 清空注册页面的数据
            ui->resgin_usr->clear();
            ui->resgin_nickname->clear();
            ui->resgin_pwd->clear();
            ui->resgin_pwd_con->clear();
            ui->resgin_email->clear();
            ui->resgin_phone->clear();
            ui->re_status->clear();

            // 设置登录消息
            ui->sginin_usr->setText(user);
            ui->sginin_pwd->setText(pwd);
            ui->rember_pwd->setChecked(true);

            // 切换到登录页面
            ui->stackedWidget->setCurrentWidget(ui->sginin_page);
        }
        else if(code == "011")
        {
            ui->re_status->setText(QString("注册失败,[%1]该用户已经存在!").arg(user));
        }
        else if(code == "012")
        {
            ui->re_status->setText(QString("注册失败,[%1]该手机已经注册!").arg(phone));
        }
        else if(code == "013")
        {
            ui->re_status->setText(QString("注册失败,[%1]该邮箱已经注册!").arg(email));
        }
        else if(code == "014")
        {
            ui->re_status->setText(QString("注册失败,[%1]该名称已被占用!").arg(phone));
        }else
        {
            ui->re_status->setText("出现错误");
        }

        // 释放资源
        delete reply;
    });
}

void Login::on_ser_ok_btn_clicked()
{
    QString ip = ui->server_ip->text();
    QString port = ui->server_port->text();

    // 数据判断
    // 服务器IP
    // \\d 和 \\. 中第一个\是转义字符, 这里使用的是标准正则
    QRegExp regexp(IP_REG);
    if(!regexp.exactMatch(ip))
    {
        ui->server_status->setText("您输入的IP格式不正确, 请重新输入!");
        return;
    }
    // 端口号
    regexp.setPattern(PORT_REG);
    if(!regexp.exactMatch(port))
    {
        ui->server_port->setText("您输入的端口格式不正确, 请重新输入!");
        return;
    }

    // 切换到登录页面
    ui->stackedWidget->setCurrentWidget(ui->sginin_page);

    // 将服务器相关信息写入配置文件中
    m_common.writeWebInfo(ip,port);
}


void Login::readCfg()
{
    QString user = m_common.getConfvalue("login","user");
    QString pwd = m_common.getConfvalue("login","pwd");
    QString remember = m_common.getConfvalue("login","remember");

    int ret = 0;
    if(remember == "yes")    // 記住密碼
    {
        // 解开密码的密钥
        QByteArray pwdinfo =  m_aeskey->decode(QByteArray::fromBase64(pwd.toLocal8Bit()));

#ifdef _WIN32  // 如果是windows平台
        ui->sginin_pwd->setText(QString::fromLocal8Bit(pwdinfo));
#else
        ui->sginin_pwd->setText(key.data());
#endif
        // 将密码保存按钮置为true
        ui->rember_pwd->setChecked(true);
    }
    else
    {
        ui->sginin_pwd->setText("");
        ui->rember_pwd->setChecked(false);
    }

    // 解除
    QByteArray userinfo = m_aeskey->decode(QByteArray::fromBase64(user.toLocal8Bit()));

#ifdef _WIN32  // 如果是windows平台
        ui->sginin_usr->setText(QString(QString::fromLocal8Bit(userinfo)));
#else
        ui->sginin_usr->setText(userinfo.data());
#endif
    // 将网络配置的数据显示出来
    QString ip = m_common.getConfvalue("server","ip");
    QString port = m_common.getConfvalue("server","port");

    ui->server_ip->setText(ip);
    ui->server_port->setText(port);
}

void Login::paintEvent(QPaintEvent *event)
{
    // 给窗口画背景图
    QPainter p(this);
    QPixmap pixmap(":/image/title_bk.jpg");
    p.drawPixmap(0,0,this->width(),this->height(),pixmap);
}
