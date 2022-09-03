#include "myfile.h"
#include "qjsonarray.h"
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif
#include "ui_myfile.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QAction>
#include "common/logininfoinstance.h"
#include "selfwidget/dataprogress.h"
#include "common/downdask.h"
#include "selfwidget/fileproperty.h"

MyFile::MyFile(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MyFile)
{
    ui->setupUi(this);

    // 初始化对象网络管理对象
    m_manager = Common::getNetManager();

    initListWidget();       //  初始化listWidget文件列表

    addActionMenu();        // 添加右键菜单

    checkTaskList();
}

MyFile::~MyFile()
{
    // 清空文件列表
    clearFileList();

    // 清空所有item项目
    clearItems();

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

    // 列表中的上传文件图标
    connect(ui->listWidget,&QListWidget::itemPressed,this,[=](QListWidgetItem* item)
    {
        QString str = item->text();
        if(str == "上传文件")
        {
            // 添加需要上传的文件到上传任务列表
            addUploadFiles();
        }
    });
}

void MyFile::addActionMenu()
{
    // ===============菜单1====================
    m_menu = new QMenu(this);

    // 初始化菜单
    m_downloadAction = new QAction("下载",this);
    m_shareAction = new QAction("分享",this);
    m_cancelAction = new QAction("取消分享",this);
    m_delAction = new QAction("删除",this);
    m_propertyAction = new QAction("属性", this);


    // 动作1添加到菜单1
    m_menu->addAction(m_downloadAction);
    m_menu->addAction(m_shareAction);
    m_menu->addAction(m_cancelAction);
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
        processSelectedFiles("下载");
    });

    // 分享
    connect(m_shareAction, &QAction::triggered, [=]()
    {
        cout << "分享动作";
        processSelectedFiles("分享"); //处理选中的文件
    });

    // 取消分享
    connect(m_cancelAction, &QAction::triggered, [=]()
    {
        cout << "取消分享动作";
        processSelectedFiles("取消分享"); //处理选中的文件
    });

    // 删除
    connect(m_delAction, &QAction::triggered, [=]()
    {
        cout << "删除动作";
        processSelectedFiles("删除"); //处理选中的文件
    });

    // 属性
    connect(m_propertyAction, &QAction::triggered, [=]()
    {
        cout << "属性动作";
        processSelectedFiles("属性"); //处理选中的文件
    });


    // 菜单二
    // 按下载量升序
    connect(m_pvAscendingAction, &QAction::triggered, [=]()
    {
        cout << "按下载量升序";
        refreshFile(PvAsc);
    });

    // 按下载量降序
    connect(m_pvDescendingAction, &QAction::triggered, [=]()
    {
        cout << "按下载量降序";
        refreshFile(PvDesc);
    });

    //刷新
    connect(m_refreshAction, &QAction::triggered, [=]()
    {
        cout << "刷新动作";
        //显示用户的文件列表
        refreshFile();
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

    QNetworkRequest request;
    QString url = QString("http://%1:%2/md5").arg(login->getIp()).arg(login->getPort());
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
        if(rely->error() != QNetworkReply::NoError)         //   在服务器段追加双重保障 MD5服务器访失败直接进行正常上传
        {
            cout << rely->errorString();
            rely->deleteLater();
            // 删除任务
            uploadList->dealUploadTask();

            QMessageBox::warning(this,"上传出错","服务器请求失败！");
            return ;
        }

        // 服务器返回数据
        QByteArray array = rely->readAll();
        rely->deleteLater();

        /*
        秒传文件：

            秒传成功：     {"code":"100"}
            文件不在存储区：{"code":"110"}
            文件存在存储区：{"code":"112"}
            出错：        {"code":"404"}
            token验证失败：{"code":"400"}

        */

        QString retcode = m_common.getCode(array);

        // =============服务器中没有该文件执行正常上传================

        if(retcode == "110")
        {
            cout << "MD5 秒传失败，执行正常上传";

            uploadFile(upload);                                     // 说明后台服务器没有该文件，需要正常上传
        }
        else if(retcode == "111")
        {
            cout << "文件存在";                                     // 文件存在文件列表中

            // 将上传状态记录在系统中
            m_common.writeRecord(login->getUser(),upload->filename,retcode);

            // 删除已经传递完成的上传任务
            uploadList->dealUploadTask();
        }
        else if(retcode == "112")
        {
            cout << "MD5 秒传成功";                                 // 秒传成功

            // 将上传状态记录在系统中
            m_common.writeRecord(login->getUser(),upload->filename,retcode);

            // 删除已经传递完成的上传任务
            uploadList->dealUploadTask();

            // 刷新主页面
            refreshFile();
        }
        else if(retcode == "400")
        {
            QMessageBox::warning(this,"账号异常","请重新登录！！");
            emit loginAgainSignal();                                // 发送重新登录信号

            // 删除已经传递完成的上传任务
            uploadList->dealUploadTask();

            return ;
        }
        else
        {
            cout << "上传出现错误";                                   // 上传失败
            // 将上传状态记录在系统中
            m_common.writeRecord(login->getUser(),upload->filename,retcode);

            // 删除已经传递完成的上传任务
            uploadList->dealUploadTask();
        }
    });
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

    // 上传信息
    QNetworkRequest request;
    QString url = QString("http://%1:%2/uploadfile").arg(login->getIp(),login->getPort());
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");


    data.append(boundary);                         //边界头
    data.append("\r\n");

    data.append("Content-Disposition: form-data;");          // 文件传输类型
    //data.append(QString("user=\"%1\"; filename=\"%2\"; md5=\"%3\"; size=%4") \
                .arg(login->getUser()).arg(upload->filename).arg(upload->md5).arg(upload->size));
    data.append( QString("user=\"%1\" ").arg( login->getUser() ) ); //上传用户
    data.append( QString("filename=\"%1\" ").arg(upload->filename) ); //文件名字
    data.append( QString("md5=\"%1\" ").arg(upload->md5) ); //文件md5码
    data.append( QString("size=%1").arg(upload->size)  );   //文件大小
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

    // 获取请求
    QNetworkReply *reply = m_manager->post(request,data);
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }


    // 修改进度模块数值
    connect(reply,&QNetworkReply::uploadProgress,[=](qint64 bytesRead, qint64 totalBytes)
    {
        if(totalBytes != 0) // 判断条件
        {
            upload->dp->setProgress(bytesRead/1024,totalBytes/1024);  //  设置进度
        }
    });

    //获取上传列表实例
    UpLoadTask *uploadList = UpLoadTask::getInstance();
    if(uploadList == NULL)
    {
        cout << "UploadTask::getInstance() == NULL";
        return;
    }

    // 对传输完成后的数据进行处理
    connect(reply,&QNetworkReply::finished,[=]()
    {
        if(reply->error() != QNetworkReply::NoError)
        {
            cout << reply->errorString();
            reply->deleteLater();

            uploadList->dealUploadTask();
            return ;
        }

        QByteArray array = reply->readAll();
        reply->deleteLater();

        // 将获取到的状态写入本地
        QString retcode = m_common.getCode(array);
        if(retcode == "113")
        {
            cout << upload->filename.toLocal8Bit().data() << "=>上传成功";           // 提示上传成功
            // 刷新页面
            refreshFile();
        }
        else
        {
            cout << upload->filename.toLocal8Bit().data() << "=>上传失败";           // 提示上传失败
        }

        m_common.writeRecord(login->getUser(),upload->filename.toUtf8(),retcode);        // 将状态写入系统记录中

        uploadList->dealUploadTask();
    });
}



void MyFile::refreshFileItems()
{
    // 清理所有item
    clearItems();

    if(m_fileList.isEmpty() == false)
    {
        for(auto val: m_fileList)
        {
            // 获取文件数据的item
            QListWidgetItem *item = val->item;
            ui->listWidget->addItem(item);
        }
    }

    // 添加上传文件的对象item        再文件末尾
    this->addUploadItem();
}

void MyFile::addUploadItem(QString iconPath, QString name)
{
    // 创建对象
    QListWidgetItem *item = new QListWidgetItem(QIcon(iconPath),name);
    // 添加item对象
    ui->listWidget->addItem(item);
}

void MyFile::clearFileList()
{
    for(auto val : m_fileList)
    {
        delete val;                     // 释放所有对象

    }
    m_fileList.clear();                 // 清除列表中所有节点
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

QByteArray MyFile::setDealFileJson(QString user, QString token, QString filename, QString md5)
{
    QMap<QString,QVariant> map;
    map.insert("user_name",user);
    map.insert("token",token);
    map.insert("file_name",filename);
    map.insert("md5",md5);

    QJsonDocument doc = QJsonDocument::fromVariant(map);
    if(doc.isNull() || doc.isEmpty())
    {
        cout << "doc.isNull() || doc.isEmpty()";
        return NULL;
    }

    return doc.toJson();
}

QByteArray MyFile::setMd5Json(QString user, QString token, QString md5, QString filename)
{
    QMap<QString,QVariant> tmp;
    tmp.insert("user_name",user);
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

QByteArray MyFile::setFilesListJson(QString user, QString token, int start, int count)
{
    QMap<QString,QVariant> tmp;
    tmp.insert("user_name",user);
    tmp.insert("token",token);
    tmp.insert("start",start);
    tmp.insert("count",count);

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(tmp);
    if(jsonDocument.isNull() == true)
    {
        cout << "jsonDocument.isNull() == true";
        return "";
    }
    return jsonDocument.toJson();
}

QString MyFile::getCountStatus(QByteArray json)
{
    QJsonParseError error;

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
        if(ret == "400")
        {
            QMessageBox::warning(this,"账户异常","请重新登录！！");
            emit loginAgainSignal();
            return "";
        }else if(ret == "100")
            return QString(doc.object().value("count").toString());
        else if(ret == "105")
            cout << "没有文件";
    }
    return "0";
}

void MyFile::refreshFile(Display cmd)
{
    m_userFilesCount= 0;

    QNetworkRequest request;

    LoginInfoInstance *login = LoginInfoInstance::getInstance();

    QString url = QString("http://%1:%2/gainfile?cmd=count").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

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
        QByteArray data = reply->readAll();

        reply->deleteLater();

        // 获取服务器json文件
        QString ret = getCountStatus(data);
        if(ret == "")return;

        m_userFilesCount = ret.toLong();

        cout << "userFilesCount" << m_userFilesCount;

        // 清空文件列表信息
        clearFileList();

        if(m_userFilesCount > 0)
        {
            // 获取用户文件列表
            m_start = 0;
            m_count = 10;

            // 获取新的文件列表信息
            getUserFilesList(cmd);

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
    if(data == "")return;

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
        if(retcode == "400")
        {
            QMessageBox::warning(this,"账户异常","请重新登录！！");
            emit loginAgainSignal();
            return;
        }
        else if(retcode == "100")
        {
            getFileJsonInfo(array);

            // 继续获取用户列表
            getUserFilesList(cmd);
        }
    });
}

void MyFile::getFileJsonInfo(QByteArray data)
{
    QJsonParseError error;

    QJsonDocument doc = QJsonDocument::fromJson(data,&error);
    if(error.error != QJsonParseError::NoError)
    {
        cout << "err = " << error.errorString();
        return ;
    }

    if(doc.isNull() || doc.isEmpty())
    {
        cout << "doc.isNull() || doc.isEmpty()";
        return;
    }


    /*
        //文件信息
        struct FileInfo
        {
            QString md5;            // 文件md5码
            QString fileName;       // 文件名字
            QString user;           // 用户
            QString createTime;     // 上传时间
            QString url;            // url
            QString type;           // 文件类型
            qint64 size;            // 文件大小
            int shareStatus;        // 是否共享, 1共享， 0不共享
            int pv;                 // 下载量
            QListWidgetItem *item;  // list widget 的item
        };

        {
        "user": "milo",
        "md5": "e8ea6031b779ac26c319ddf949ad9d8d",
        "create_time": "2020-06-21 21:35:25",
        "file_name": "test.mp4",
        "share_status": 0,
        "pv": 0,
        "url": "http://192.168.52.139:80/group1/M00/00/00/wKgfbViy2Z2AJ-FTAaDSAE-g3Z0782.mp4",
        "size": 27473666,
         "type": "mp4"
        }
    */

    if(doc.isObject())
    {
        QJsonObject obj = doc.object();                             // 将json转化成对象
        QJsonArray array = obj.value("files").toArray();            // 讲files字段的所有数据转化成jsonarray对象

        for(auto val : array)
        {
            QJsonObject tmp = val.toObject();

            FileInfo *info = new FileInfo;                          // 创建文件对象
            info->user = tmp.value("user_name").toString();
            info->md5 = tmp.value("md5").toString();
            info->createTime = tmp.value("create_time").toString();
            info->fileName = tmp.value("file_name").toString();
            info->shareStatus = tmp.value("share_status").toInt();
            info->pv = tmp.value("pv").toInt();
            info->url = tmp.value("url").toString();
            info->size = tmp.value("size").toInt();
            info->type = tmp.value("type").toString();

            QString type = info->type + ".png";
            info->item = new QListWidgetItem(QIcon(m_common.getFileType(type)),info->fileName);         // 创建文件item

            // 添加到文件列表
            m_fileList.append(info);

        }

    }
}

void MyFile::processSelectedFiles(QString cmd)
{
    QListWidgetItem *item = ui->listWidget->currentItem();          // 获取被选中的item对象
    if(item == nullptr)
    {
        cout << "未选中文件，出错";
        return;
    }

    for(auto val : m_fileList)
    {
        if(val->item == item)
        {
            if(cmd == "删除")
                delFile(val);
            else if(cmd == "下载")
                downFile(val);
            else if(cmd == "属性")
                getFileProperty(val);
            else if(cmd == "分享")
                shareFile(val);
            else if(cmd == "取消分享")
                dealshareFile(val);

            break;
        }
    }
}

void MyFile::delFile(FileInfo *info)
{

    LoginInfoInstance *login = LoginInfoInstance::getInstance();
    QNetworkRequest request;
    QString url = QString("http://%1:%2/ctl?cmd=del").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray array = setMd5Json(login->getUser(),login->getToken(),info->md5,info->fileName);

    QNetworkReply *reply = m_manager->post(request,array);
    if(reply == nullptr)
    {
        cout << "reply == nullptr";
        return;
    }

    connect(reply,&QNetworkReply::finished,[=]()
    {
         if(reply->error() != QNetworkReply::NoError)
         {
             cout << reply->errorString();
             reply->deleteLater();
             return;
         }

         QByteArray array = reply->readAll();
         reply->deleteLater();

         QString retcode = m_common.getCode(array);
         if(retcode == "400")
         {
            QMessageBox::warning(this, "账户异常", "请重新登陆！！！");
            emit loginAgainSignal(); //发送重新登陆信号
            return;
         }
         else if(retcode == "100")
         {
            QMessageBox::information(this, "文件删除", QString("[%1] 文件删除成功").arg(info->fileName));

            // 删除listWidget上的item
            ui->listWidget->removeItemWidget(info->item);
            delete info->item;

            // 删除文件列表中的指定对象
            m_fileList.removeOne(info);
            delete info;
         }
         else
         {
             QMessageBox::information(this, "文件删除", QString("[%1] 删除失败").arg(info->fileName));
         }
    });

}

void MyFile::downFile(FileInfo *info)
{
    // 跳转下载页面
    emit gotoTransfer(TransferStatus::Download);

    DownDask *tmp = DownDask::getInstance();
    if(tmp == nullptr)
    {
        cout << "DownDask::getInstance()";
        return;
    }

    QString savePath = QFileDialog::getSaveFileName(this,"请选择存储路径",info->fileName);

    if(savePath.isEmpty() == true)
    {
        cout << "获取的文件存储路径出错";
        return;
    }

    // 将下载任务加入到下载任务中去
    int ret = tmp->appendDownloadList(info,savePath);
    if(ret == -2)
        QMessageBox::warning(this,"任务已存在","任务已存在下载队列中！！");
    else if(ret == -3)
        cout << "打开" << info->fileName << "文件失败";       //  下载文件失败，记录
    else if(ret == -4)
        cout << "获取布局失败";

}

void MyFile::getFileProperty(FileInfo *info)
{
    FileProperty dlg; //创建对话框
    dlg.setText(info);

    dlg.exec(); //模态方式运行
}

void MyFile::shareFile(FileInfo *info)
{
    // 获取当前用户数据
    LoginInfoInstance *login = LoginInfoInstance::getInstance();

    // 形成json数据

    QNetworkRequest request;
    QString url = QString("http://%1:%2/ctl?cmd=share").arg(login->getIp()).arg(login->getPort());

    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray array = setDealFileJson(login->getUser(),login->getToken(),info->fileName,info->md5);

    QNetworkReply *reply = m_manager->post(request,array);
    if(reply == nullptr)
    {
        cout << "reply == nullptr";
        return;
    }


    connect(reply,&QNetworkReply::finished,[=]()
    {
        if(reply->error() != QNetworkReply::NoError)
        {
            cout << reply->errorString();
            return;
        }

        /*
            分享文件：
                成功：{"code":"100"}
                别人已经分享此文件：{"code", "106"}
                失败：{"code":"404"}

            token验证失败：{"code":"400"}

            */
        QString retcode = m_common.getCode(reply->readAll());
        if(retcode == "400")
        {
            QMessageBox::warning(this, "账户异常", "请重新登陆！！！");
            emit loginAgainSignal(); //发送重新登陆信号

            return;
        }
        else if(retcode == "100")
        {
            //修改文件列表的属性信息
            info->shareStatus = 1; //设置此文件为已分享
            QMessageBox::information(this, "分享", QString("[%1] 分享成功").arg(info->fileName));

            // 刷新文本目录
//            refreshFile();
        }
        else if(retcode == "106")
        {
            QMessageBox::information(this, "分享", QString("[%1] 别人已分享此文件").arg(info->fileName));
        }
        else
        {
            QMessageBox::information(this, "分享", QString("[%1] 分享失败").arg(info->fileName));
        }
    });
}

void MyFile::dealshareFile(FileInfo *info)
{
   LoginInfoInstance *login = LoginInfoInstance::getInstance();
   QNetworkRequest request;
   QString url = QString("http://%1:%2/ctl?cmd=delshare").arg(login->getIp()).arg(login->getPort());
   request.setUrl(QUrl(url));
   request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

   QByteArray array = setDealFileJson(login->getUser(),login->getToken(),info->fileName,info->md5);
   QNetworkReply *reply = m_manager->post(request,array);

   connect(reply,&QNetworkReply::finished,[=]()
   {
        if(reply->error() != QNetworkReply::NoError)
        {
            cout << reply->errorString();
            reply->deleteLater();
            return;
        }

        QByteArray array = reply->readAll();
        reply->deleteLater();

        QString retcode = m_common.getCode(array);

        /*
            分享文件：
                成功：{"code":"100"}
                失败：{"code":"404"}

            token验证失败：{"code":"400"}

            */
        if(retcode == "400")
        {
            QMessageBox::warning(this, "账户异常", "请重新登陆！！！");
            emit loginAgainSignal();
            return;
        }
        else if(retcode == "100")
        {
            info->shareStatus = 0;

            QMessageBox::information(this, "取消分享", QString("[%1]取消分享成功").arg(info->fileName));
        }
        else
           QMessageBox::information(this, "取消分享", QString("[%1]取消分享失败").arg(info->fileName));
   });
}

void MyFile::downloadFilesAction()
{
    DownDask* downDask = DownDask::getInstance();
    if(downDask == nullptr)
    {
         cout << "DownloadTask::getInstance() == nullptr";
         return;
    }

    // 判断下载任务队列中是否有任务
    if(downDask->isEmpty() == true)return;

    // 判断任务是否正在处理下载
    if(downDask->isDownload() == true)return;

    // 查看文件是否为共享
    if(downDask->isShareTask() == true)return;

    DownladoInfo *tmp = downDask->takeTask();           // 获取第一个任务

    QUrl url = tmp->url;
    QFile *file = tmp->file;
    QString md5 = tmp->md5;
    QString user = tmp->user;
    QString filename = tmp->filename;
    DataProgress *dp = tmp->dp;

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    //发送get请求
    QNetworkReply * reply = m_manager->get(request);
    if(reply == NULL)
    {
        downDask->dealDownloadTask(); //删除任务
        cout << "get err";
        return;
    }

    //获取请求的数据完成时，就会发送信号SIGNAL(finished())
    connect(reply, &QNetworkReply::finished,[=]()
    {
//        if(reply->error() != QNetworkReply::NoError)
//        {
//            cout << reply->errorString();
//            reply->deleteLater();

//            downDask->dealDownloadTask();//删除下载任务
//            QMessageBox::warning(this,"下载文件","文件下载失败");
//            return;
//        }
        cout << "下载完成";
        reply->deleteLater();

        downDask->dealDownloadTask();//删除下载任务
        m_common.writeRecord(user, filename, "100"); //下载文件成功，记录

        dealFilePv(md5, filename); //下载文件pv字段处理
    });

    //当有可用数据时，reply 就会发出readyRead()信号，我们这时就可以将可用的数据保存下来
    connect(reply, &QNetworkReply::readyRead,[=]()
    {
        //如果文件存在，则写入文件
        if(file != nullptr)
        {
            file->write(reply->readAll());
        }
    });

    //有可用数据更新时
    connect(reply, &QNetworkReply::downloadProgress,[=](qint64 bytesRead, qint64 totalBytes)
    {
        dp->setProgress(bytesRead, totalBytes);//设置进度
    }
    );
}

void MyFile::dealFilePv(QString md5, QString filename)
{
    // 获取当前用户信息
    LoginInfoInstance *login = LoginInfoInstance::getInstance();

    QNetworkRequest request;
    QString url = QString("http://%1:%2/ctl?cmd=pv").arg(login->getIp(),login->getPort());
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray array = setDealFileJson(login->getUser(),login->getToken(),filename,md5);


    // 发送请求
    QNetworkReply *reply = m_manager->post(request,array);

    connect(reply,&QNetworkReply::finished,[=]()
    {
        if(reply->error() != QNetworkReply::NoError)
        {
            cout << reply->errorString();
            reply->deleteLater();
            return;
        }
        // 获取数据判断结果
        QByteArray ret = reply->readAll();
        reply->deleteLater();

        QString retcode = m_common.getCode(ret);
        if(retcode == "100")
        {
            // 任务队列已经删除任务了所以需要便利查找
            for(auto val : m_fileList)
            {
                if(val->fileName == filename && val->md5 == md5)
                {
                    val->pv += 1;
                    break;
                }
            }
        }
        else
        {
            cout << "下载文件pv字段处理失败";
        }
    });
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
        downloadFilesAction();
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

        // 设置全局共享文件按钮
        m_shareAction->setEnabled(true);
        // 设置全局取消共享文件按钮
        m_cancelAction->setEnabled(false);

        for(auto val : m_fileList)
        {
            if(item == val->item)
            {
                if(val->shareStatus == 1)
                {
                    m_shareAction->setEnabled(false);      // 只有是共想文件才关闭
                    m_cancelAction->setEnabled(true);      // 只有是共想文件才能开启
                }
            }
        }

        if(item->text() == "上传文件") //如果是上传文件，没有右击菜单
        {
            return;
        }

        m_menu->exec( QCursor::pos() );
    }
}
