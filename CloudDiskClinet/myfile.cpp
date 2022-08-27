#include "myfile.h"
#include "ui_myfile.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QAction>
#include "common/logininfoinstance.h"
#include "selfwidget/dataprogress.h"

MyFile::MyFile(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MyFile)
{
    ui->setupUi(this);

    // 初始化对象网络管理对象
    m_manager = Common::getNetManager();

    initListWidget();       //  初始化listWidget文件列表

    addActionMenu();        // 添加右键菜单
}

MyFile::~MyFile()
{
    delete ui;
}

void MyFile::initListWidget()
{
    ui->listWidget->setViewMode(QListView::IconMode);       // 设置显示图标
    ui->listWidget->setIconSize(QSize(80,80));              // 设置图标大小
    ui->listWidget->setGridSize(QSize(100,120));            // 设置item大小

    // 设置QListView大小改变时，图标的调整模式，默认固定的，可以改成自动调整
    ui->listWidget->setResizeMode(QListView::Adjust);       //  自动适应布局

    // 设置列表可以拖动，乳沟想要固定不能拖动，使用QlistView::Static
    ui->listWidget->setMovement(QListView::Static);

    // 设置图标之间的间距，当setGridSize()时，此选项无效
    ui->listWidget->setSpacing(30);

    // listWidget 右键菜单
    // 发送customContexMenuRequested信号
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidget,&QListWidget::customContextMenuRequested,this,&MyFile::rightMenu);
//    // 点钟列表中的上传文件图标
//    connect(ui->listWidget,&QListWidget::itemPressed,this,[=](QListWidgetItem* item)
//    {
//        QString str = item->text();
//        if(str == "上传文件")
//        {
//            // 添加需要上传的文件到上传任务列表
//            addUploadFiles();
//        }
//    });
}

void MyFile::addActionMenu()
{
    // ===============菜单1====================
    m_menu = new QMenu(this);

    // 初始化菜单
    m_downloadAction = new QAction("下载",this);
    m_shareAction = new QAction("分享",this);
    m_delAction = new QAction("删除",this);
    m_propertyAction = new QAction("属性", this);

    // 动作1添加到菜单1
    m_menu->addAction(m_downloadAction);
    m_menu->addAction(m_shareAction);
    m_menu->addAction(m_delAction);
    m_menu->addAction(m_propertyAction);

    // ===============菜单2====================
    m_menuEmpty = new QMenu( this );
    // 动作2
    m_pvAscendingAction = new QAction("按下载量升序", this);
    m_pvDescendingAction = new QAction("按下载量降序", this);
    m_refreshAction = new QAction("刷新", this);
    m_uploadAction = new QAction("上传", this);

    // 动作2添加到菜单2
    m_menuEmpty->addAction(m_pvAscendingAction);
    m_menuEmpty->addAction(m_pvDescendingAction);
    m_menuEmpty->addAction(m_refreshAction);
    m_menuEmpty->addAction(m_uploadAction);



    // ===========信号与槽=================
    // 菜单一
    // 下载
    connect(m_downloadAction, &QAction::triggered, [=]()
    {
        cout << "下载动作";
        //添加需要下载的文件到下载任务列表
//        addDownloadFiles();
    });

    // 分享
    connect(m_shareAction, &QAction::triggered, [=]()
    {
        cout << "分享动作";
//        dealSelectdFile("分享"); //处理选中的文件
    });

    // 删除
    connect(m_delAction, &QAction::triggered, [=]()
    {
        cout << "删除动作";
//        dealSelectdFile("删除"); //处理选中的文件
    });

    // 属性
    connect(m_propertyAction, &QAction::triggered, [=]()
    {
        cout << "属性动作";
//        dealSelectdFile("属性"); //处理选中的文件
    });
    // 菜单二
    // 按下载量升序
    connect(m_pvAscendingAction, &QAction::triggered, [=]()
    {
        cout << "按下载量升序";
//        refreshFiles(PvAsc);
    });

    // 按下载量降序
    connect(m_pvDescendingAction, &QAction::triggered, [=]()
    {
        cout << "按下载量降序";
//        refreshFiles(PvDesc);
    });

    //刷新
    connect(m_refreshAction, &QAction::triggered, [=]()
    {
        cout << "刷新动作";
        //显示用户的文件列表
//        refreshFiles();
    });

    //上传
    connect(m_uploadAction, &QAction::triggered, [=]()
    {
        cout << "上传动作";
        //添加需要上传的文件到上传任务列表
        addUploadFiles();
    });
}

void MyFile::addUploadFiles()
{
    // 发送信号
    emit gotoTransfer(TransferStatus::Uplaod);

    // 获取上传列表实例
    UpLoadTask *uploadlist = UpLoadTask::getInstance();

    if(uploadlist == nullptr)
    {
        cout << "UpLoadTask::getInstance()";
        return;
    }

    // 打开窗口选择文件
    QStringList list_z = QFileDialog::getOpenFileNames();

    for(auto val : list_z)
    {
         qDebug() << val ;
        int ret = uploadlist->appendUploadList(val);

        if(ret == -2)
            QMessageBox::warning(this,"添加失败","上传的文件是否已存在上传列表中！！");
        else if(ret == -3)
            cout << "打开" << val << "文件失败";
        else if(ret == -4)
            cout << "获取布局失败";
    }

}

void MyFile::uploadFileAction()
{
    // 获取上传列表实例
    UpLoadTask *uploadList = UpLoadTask::getInstance();
    if(uploadList == NULL)
    {
        cout << "UploadTask::getInstance() == NULL";
        return ;
    }

    // 如果队列为空，没有上传任务，程序终止
    if(uploadList->isEmpty())
    {
        cout << "任务队列为空";
        return ;
    }

    // 查看是否有上传任务，单任务上传，当前有任务，不能上传
    if(uploadList->isUpload())
    {
        cout << "有任务在上传";
        return ;
    }

    // 获取登录信息
    LoginInfoInstance *login = LoginInfoInstance::getInstance();

    // ========================尝试快传===============================
    // url
    QNetworkRequest request;
    QString url = QString("http://%1:%2/md5").arg(login->getIp()).arg(login->getPort());
    // 写入请求行
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    // ========= 获取任务 =======
    UploadFileDate *upload = uploadList->takeTask();

    // 获取文件md5值
    QByteArray array = setMd5Json(login->getUser(),login->getToken(),upload->md5,upload->filename);

    // 发送post请求
    QNetworkReply *rely = m_manager->post(request,array);

    connect(rely,&QNetworkReply::finished,[=]()
    {
        if(rely->error() != QNetworkReply::NoError)
        {
            cout << rely->errorString();
            rely->deleteLater();
            return ;
        }

        // 服务器返回数据
        QByteArray array = rely->readAll();
        rely->deleteLater();

        /*
        秒传文件：
            文件已存在：{"code":"005"}
            秒传成功：  {"code":"006"}
            秒传失败：  {"code":"007"}
            token验证失败：{"code":"111"}

        */

        QString retcode = m_common.getCode(array);

        // =============服务器中没有该文件执行正常上传================
        if(retcode == "007")
        {
            // 说明后台服务器没有该文件，需要正常上传
            uploadFile(upload);             // 将任务数据传入

            // 删除任务队列上第一个任务
            uploadList->dealUploadTask();
        }
        else if(retcode == "111")
        {
            QMessageBox::warning(this,"账号异常","请重新登录！！");
            emit loginAgainSignal();        // 发送重新登录信号

            return ;
        }
        else
        {
            // 上传失败写入本地记录中
            m_common.writeRecord(login->getUser(),upload->filename,retcode);

            // 删除已经传递完成的上传任务
            uploadList->dealUploadTask();
        }

        // 不是错误码就处理文件信息

    });

}

QByteArray MyFile::setMd5Json(QString user, QString token, QString md5, QString filename)
{
    QMap<QString,QVariant> tmp;
    tmp.insert("uset_name",user);
    tmp.insert("token",token);
    tmp.insert("file_name",filename);
    tmp.insert("md5",md5);

    QJsonDocument jsondocument = QJsonDocument::fromVariant(tmp);
    if(jsondocument.isNull())
    {
        cout << " QJsonDocument::fromVariant(tmp) err";
        return "";
    }

    return jsondocument.toJson();
}

void MyFile::uploadFile(UploadFileDate *upload)
{
    // 获取当前用户信息
    LoginInfoInstance *login = LoginInfoInstance::getInstance();

    /*
    文件传输格式，此项目每次只传输一个文件
    不需要考虑多组------WebKitFormBoundary88asdgewtgewx\r\n文件分隔操作

    ------WebKitFormBoundary88asdgewtgewx\r\n
    Content-Disposition: form-data; user="milo"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    Content-Type: application/octet-stream\r\n
    \r\n
    真正的文件内容\r\n
    ------WebKitFormBoundary88asdgewtgewx
    */

    // 文件信息
    QByteArray data;

    // 生成分割线
    QString boundary = m_common.getBoundary();

    data.append(boundary);                         //边界头
    data.append("\r\n");

    data.append("Content-Disposition:form-data;");          // 文件传输类型
    data.append(QString("user=\"%1\"; filename=\"%2\"; md5=\"%3\"; size=%4") \
                .arg(login->getUser()).arg(upload->filename).arg(upload->md5).arg(upload->size));
    data.append("\r\n");

    data.append("Content-Type: application/octet-stream");
    data.append("\r\n");
    data.append("\r\n");

    // 真正上串的文件
    QFile *file = upload->file;
    data.append(file->readAll());
    data.append("\r\n");

    // 结束分割先
    data.append(boundary);
    data.append("\r\n");

    // 上传信息
    QNetworkRequest request;
    QString url = QString("http://%1:%2/uploadfile");

    // 获取请求
    QNetworkReply *reply = m_manager->post(request,data);

    // 修改进度模块数值
    connect(reply,&QNetworkReply::uploadProgress,[=](qint64 bytesRead, qint64 totalBytes)
    {
        if(totalBytes != 0) // 判断条件
        {
            upload->dp->setProgress(bytesRead/1024,totalBytes/1024);  //  设置进度
        }
    });

    // 对传输完成后的数据进行处理
    connect(reply,&QNetworkReply::finished,[=]()
    {
        if(reply->error() != QNetworkReply::NoError)
        {
            cout << reply->errorString();
            reply->deleteLater();
            return ;
        }

        QByteArray array = reply->readAll();

        reply->deleteLater();

        // 将获取到的状态写入本地
        QString retcode = m_common.getCode(array);
        m_common.writeRecord(login->getUser(),upload->filename,retcode);

    });



}

void MyFile::clearFileList()
{
    if(m_fileList.isEmpty() == true)return;
    for(auto val : m_fileList)
    {
        delete val;
    }
}

void MyFile::clearItems()
{
    int count = ui->listWidget->count();
    for(int i = 0;i < count;++i)
    {
        QListWidgetItem *item = ui->listWidget->takeItem(0);   // 注意是0
        delete item;
    }
}

void MyFile::addUploadItem(QString iconPath, QString name)
{
    // 创建对象
    QListWidgetItem *item = new QListWidgetItem(QIcon(iconPath),name);
    // 添加item对象
    ui->listWidget->addItem(item);
}

void MyFile::refreshFileItems()
{
    clearItems();
    for(auto val: m_fileList)
    {
        // 获取文件数据的item
        QListWidgetItem *item = val->item;
        ui->listWidget->addItem(item);
    }

    // 添加上传文件的对象item        再文件末尾
    this->addUploadItem();
}

void MyFile::refreshFile(Display cmd)
{
    m_userFilesCount= 0;

    QNetworkRequest request;

    LoginInfoInstance *login = LoginInfoInstance::getInstance();

    QString url = QString("http://%1:%2/gainfile?cmd=count");
    request.setUrl(QUrl(url));

    QByteArray array = m_common.setGetCountjson(login->getUser(),login->getToken());

    QNetworkReply *reply = m_manager->post(request,array);
    if(reply == nullptr)
    {
        cout << "reply == nullptr";
        reply->deleteLater();
        return;
    }

    connect(reply,&QNetworkReply::finished,[=]()
    {
        if(reply->error() != QNetworkReply::NoError)
        {
            cout << reply->errorString();
            reply->deleteLater();
            return ;
        }

        // 服务器返回数据
        QByteArray array = reply->readAll();

        reply->deleteLater();

        // 获取服务器json文件
        QString ret = getCountStatus(array);
        if(ret == nullptr)
        {
            QMessageBox::warning(this, "账户异常", "请重新登陆！！！");
            emit loginAgainSignal();            // 发送重新登录信号
            return;
        }

        m_userFilesCount = ret.toLong();
        cout << "userFilesCount" << m_userFilesCount;

        // 清空文件列表
        clearFileList();

        if(m_userFilesCount > 0)
        {
            // 获取用户文件列表
            m_start = 0;
            m_count = 10;

            // 获取新的文件列表信息
            getUserFilesList();
        }
        else
        {
            refreshFileItems();         // 更新item
        }

    });

}

void MyFile::getUserFilesList(Display cmd)
{
    if(m_userFilesCount <= 0)
    {
        cout << "获取用户文件列表条件结束";
        refreshFileItems();         // 更新item
        return;
    }
    else if(m_count > m_userFilesCount)
    {
        m_count = m_userFilesCount;
    }

    QNetworkRequest request;

    LoginInfoInstance *login = LoginInfoInstance::getInstance();

    // 获取用户文件信息 127.0.0.1:80/gainfile&cmd=normal
    // 按下载量升序 127.0.0.1:80/gainfile?cmd=pvasc
    // 按下载量降序127.0.0.1:80/gainfile?cmd=pvdesc
    QString tmp;
    if(cmd == Normal)
        tmp = "normal";
    else if(cmd == PvAsc)
        tmp = "pvasc";
    else if(cmd == PvDesc)
        tmp = "pvdesc";

    QString url = QString("http://%1:%2/gainfile?cmd=%3").arg(login->getIp()).arg(login->getPort()).arg(tmp);
    request.setUrl(QUrl(url));

    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    /*
    {
        "user": "milo"
        "token": "xxxx"
        "start": 0
        "count": 10
    }
    */
    QByteArray data = setFilesListJson(login->getUser(),login->getToken(),m_start,m_count);

    // 改变其实位置
    m_start += m_count;                         // 起始位置向后偏移
    m_userFilesCount -= m_count;                // 总个数减去获取个数


    QNetworkReply *reply = m_manager->post(request,data);
    if(reply == nullptr)
    {
        cout << "reply == nullptr";
        reply->deleteLater();
        return;
    }
    connect(reply,&QNetworkReply::finished,[=](){
        if(reply->error() != QNetworkReply::NoError)
        {
            cout << reply->errorString();
            reply->deleteLater();
            return ;
        }

        // 获取服务器返回数据
        QByteArray array = reply->readAll();
        reply->deleteLater();

        QString retcode = m_common.getCode(array);
        if(retcode == "111")
        {
            QMessageBox::warning(this,"账户异常","请重新登录！！");
            emit loginAgainSignal();
            return;
        }
        else if(retcode == "015")return;

        getFileJsonInfo(array);

        // 继续获取用户列表
        getUserFilesList(cmd);

    });
}

QString MyFile::getCountStatus(QByteArray json)
{
    QJsonParseError error;
    QStringList list;

    // 将json数据转化成doc
    QJsonDocument doc = QJsonDocument::fromJson(json);
    if(doc.isNull() || doc.isEmpty())
    {
        cout << "doc.isNull() || doc.isEmpty()";
        return "";
    }

    if(doc.isObject())
    {
        QString ret = doc.object().value("retcode").toString();
        if(ret == "111")
            return "";

    }
    return doc.object().value("count").toString();
}

QByteArray MyFile::setFilesListJson(QString user, QString token, int start, int count)
{
    QMap<QString,QVariant> tmp;
    tmp.insert("user_name",user);
    tmp.insert("token",token);
    tmp.insert("strat",start);
    tmp.insert("count",count);

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(tmp);
    if(jsonDocument.isNull() == true)
    {
        cout << "jsonDocument.isNull() == true";
        return "";
    }
    return jsonDocument.toJson();
}

void MyFile::getFileJsonInfo(QByteArray data)
{

}

// 设置定时器
void MyFile::checkTaskList()
{
    // 定时检查上传队列是否有任务需要上传
    connect(&m_uploadFileTimer,&QTimer::timeout,[=]()
    {
        // 上传文件处理，取出上传任务列表的队首任务，上传完后，再去下一个任务
        uploadFileAction();
    });

    // 启动定时器，500毫秒间隔
    // 没500毫秒，检测上传任务，每一次只能上传一个文件
    m_uploadFileTimer.start(500);

    // 定时间检查下载队列是否有任务需要下载
    connect(&m_downloadTimer,&QTimer::timeout,[=]()
    {
        // 上传文件处理，去除上传任务列表的队首任务，上传完成后，再取下一个任务
//        downloadFilesAction();
    });

    // 启动定时器，500毫秒间隔
    // 没500毫秒，检测上传任务，每一次只能上传一个文件
    m_downloadTimer.start(500);
}

void MyFile::rightMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->listWidget->itemAt(pos);

    if(item == nullptr)
    {
        // QPoint QMouseEvent::pos()   这个只是返回相对这个widget(重载了QMouseEvent的widget)的位置。
        // QPoint QMouseEvent::globalPos()  窗口坐标，这个是返回鼠标的全局坐标
        // QPoint QCursor::pos() [static] 返回相对显示器的全局坐标
        // QWidget::pos() : QPoint 这个属性获得的是当前目前控件在父窗口中的位置
        m_menuEmpty->exec( QCursor::pos() ); //在鼠标点击的地方弹出菜单
    }
    else //点图标
    {
        ui->listWidget->setCurrentItem(item);

        if(item->text() == "上传文件") //如果是上传文件，没有右击菜单
        {
            return;
        }

        m_menu->exec( QCursor::pos() );
    }
}
