#include "sharefile.h"
#include "ui_sharefile.h"
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif
#include "qjsonobject.h"
#include <QListView>
#include <QPoint>
#include <QNetworkReply>
#include <QMessageBox>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileDialog>
#include <QListWidget>
#include "selfwidget/fileproperty.h"
#include "common/logininfoinstance.h"
#include "common/downdask.h"


ShareFile::ShareFile(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ShareFile)
{
    ui->setupUi(this);

    // 初始化对象网络管理对象
    m_manger = Common::getNetManager();

    //  初始化listWidget文件列表
    initListWidget();

    // 添加右键菜单
    addActionMenu();

    checkTaskList();
}

ShareFile::~ShareFile()
{
    // 清空文件列表
    clearshareFileList();
    // 清空所有item项目
    clearItem();

    delete ui;
}

void ShareFile::initListWidget()
{
    ui->listWidget->setViewMode(QListView::IconMode);       // 是指显示格式为图片
    ui->listWidget->setIconSize(QSize(80,80));              // 图标显示大小
    ui->listWidget->setGridSize(QSize(100,120));            // 设置item的格式大小

    // 设置QLisView大小改变时，图标的调整模式，默认是固定的，可以改成自动调整
    ui->listWidget->setResizeMode(QListWidget::Adjust);     // 自动适应布局

    ui->listWidget->setMovement(QListWidget::Static);       // 设置item不可移动

    ui->listWidget->setSpacing(30);                         // 设置图标之间的间距


    // 设置右击惨淡信号
    // 发出 customContextMenuRequested 信号
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidget,&QListWidget::customContextMenuRequested,this,&ShareFile::rightMenu);

}

void ShareFile::addActionMenu()
{
    // =========菜单一===========
    m_menuItem = new QMenu(this);

    m_downloadAction = new QAction("下载",this);
    m_saveAction = new QAction("转存",this);
    m_propertyAction = new QAction("属性",this);

    m_menuItem->addAction(m_downloadAction);
    m_menuItem->addAction(m_saveAction);
    m_menuItem->addAction(m_propertyAction);

    // =========菜单二===========
    m_menuEmpty = new QMenu(this);
    m_refreshAction = new QAction("刷新",this);

    m_menuEmpty->addAction(m_refreshAction);

    // ==========信号处理=========
    // ==菜单一==
    connect(m_downloadAction,&QAction::triggered,[=]()
    {
        cout << "下载操作";
        dealSelectdFile("下载");
    });
    connect(m_saveAction,&QAction::triggered,[=]()
    {
        cout << "转存操作";
        dealSelectdFile("转存");
    });
    connect(m_propertyAction,&QAction::triggered,[=]()
    {
        cout << "属性操作";
        dealSelectdFile("属性");

    });

    // ==菜单一==
    connect(m_refreshAction,&QAction::triggered,[=]()
    {
        cout << "刷新操作";
        refreshFiles();
    });

}

void ShareFile::refreshFileItems()
{
    clearItem();                                    // 清除当前显示item

    for(auto val : m_shareFileList)
    {
        ui->listWidget->addItem(val->item);
    }
}

void ShareFile::clearshareFileList()
{
    for(auto val : m_shareFileList)
    {
        delete val;                                     // 释放文件对象
    }

    m_shareFileList.clear();                                 // 清除列表中所有节点
}

void ShareFile::clearItem()
{
    int count = ui->listWidget->count();
    for(int i = 0;i < count;++i)
    {
        QListWidgetItem *item = ui->listWidget->takeItem(0);   // 注意是0
        delete item;
    }
}

void ShareFile::refreshFiles()
{
    m_shareFilesCount = 0;

    LoginInfoInstance *login = LoginInfoInstance::getInstance();

    QNetworkRequest request;
    QString url = QString("http://%1:%2/sharefile?cmd=count").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray array = m_common.setGetCountjson(login->getUser(),login->getToken());

    QNetworkReply *reply = m_manger->post(request,array);

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

        // 清空文件列表
        clearshareFileList();

        m_shareFilesCount = getCountStatus(array).toLong();
        cout << "shareFilesCount" << m_shareFilesCount;

        if(m_shareFilesCount > 0)
        {
            m_start = 0;
            m_count = 10;

            getUserFilesList();                                 // 获取共享文件列表
        }
        else
        {
            refreshFileItems();                                 // 更新item
        }
    });
}

void ShareFile::getUserFilesList(Display cmd)
{
    if(m_shareFilesCount <= 0)
    {
        cout << "文件获取结束";
        refreshFileItems();                                     // 显示item
        return;
    }
    else if(m_count > m_shareFilesCount)                         // 每次获取10个 当剩下文件数小于获取数时更改为生下数量
    {
        m_count = m_shareFilesCount;                              // 修改获取文件个数
    }

    LoginInfoInstance *login = LoginInfoInstance::getInstance();

    QString tmp;
    if(cmd == Normal)
        tmp = "normal";
    else if(cmd == PvDesc)
        tmp = "pvdesc";

    QNetworkRequest request;
    QString url = QString("http://%1:%2/sharefile?cmd=%3").arg(login->getIp()).arg(login->getPort()).arg(tmp);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray array = setFilesListJson(login->getUser(),login->getToken(),m_start,m_count);
    if(array == "")return;

    // 改变其实位置
    m_start += m_count;                         // 起始位置向后偏移
    m_shareFilesCount -= m_count;                // 总个数减去获取个数

    QNetworkReply *reply = m_manger->post(request,array);

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

            getUserFilesList();
        }
        else if(retcode == "105")
            cout << "没有文件";
    });

}

void ShareFile::getFileJsonInfo(QByteArray data)
{
    QJsonParseError error;

    QJsonDocument doc = QJsonDocument::fromJson(data,&error);
    if(error.error != QJsonParseError::NoError)
    {
        cout << "err=" << error.errorString();
        return ;
    }
    if(doc.isNull() == true || doc.isEmpty() == true)
    {
        cout << "doc.isNull() == true || doc.isEmpty() == true";
        return ;
    }

    if(doc.isObject() == true)
    {
        QJsonArray array = doc.object().value("files").toArray();

        for(auto val : array)
        {
            QJsonObject obj = val.toObject();               // 获取对象

            FileInfo *tmp = new FileInfo;                   // 创建数据结构
            tmp->user = obj.value("user_name").toString();
            tmp->md5 = obj.value("md5").toString();
            tmp->createTime = obj.value("create_time").toString();
            tmp->fileName = obj.value("file_name").toString();
            tmp->pv = obj.value("pv").toInt();
            tmp->url = obj.value("url").toString();
            tmp->size = obj.value("size").toInt();
            tmp->type = obj.value("type").toString();
            tmp->shareStatus = 1;

            QString type = tmp->type + ".png";
            tmp->item = new QListWidgetItem(QIcon(m_common.getFileType(type)),tmp->fileName);         // 创建文件item

            // 将文件添加到链表中
            m_shareFileList.push_back(tmp);
        }
    }

}


QString ShareFile::getCountStatus(QByteArray json)
{
    QJsonParseError error;

    QJsonDocument doc = QJsonDocument::fromJson(json,&error);

    if(error.error != QJsonParseError::NoError)
    {
        cout << "err = " << error.errorString();
        return "";
    }

    if(doc.isEmpty() == true || doc.isNull() == true)
    {
        cout << "doc.isNull() == true || doc.isEmpty() == true";
        return "";
    }

    if(doc.isObject() == false)return "";

    QJsonObject obj = doc.object();
    QString retcode = obj.value("retcode").toString();
    if(retcode == "400")
    {
        QMessageBox::warning(this,"账户异常","请重新登录！！");
        emit loginAgainSignal();
        return "";
    }
    else if(retcode == "100")
        return obj.value("count").toString();

    return "0";
}

QByteArray ShareFile::setFilesListJson(QString user, QString token, int start, int count)
{
    QMap<QString,QVariant> tmp;
    tmp.insert("user_name",user);
    tmp.insert("token",token);
    tmp.insert("start",start);
    tmp.insert("count",count);

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(tmp);
    if(jsonDocument.isEmpty() == true)
    {
        cout << "jsonDocument.isEmpty() == true";
        return "";
    }
    return jsonDocument.toJson();
}

QByteArray ShareFile::setDealFileJson(QString user, QString token, QString filename, QString md5)
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

void ShareFile::dealSelectdFile(QString cmd)
{
    QListWidgetItem *item = ui->listWidget->currentItem();
    if(item == nullptr)
    {
        cout << "item == nullptr";
        return ;
    }
    for(auto val : m_shareFileList)
    {
        if(val->item == item)
        {
            if(cmd == "下载")
                downFile(val);
            else if(cmd == "转存")
                saveFile(val);
            else if(cmd == "属性")
                getFileProperty(val);

            break;          // 跳出循环
        }
    }

}

void ShareFile::downFile(FileInfo *info)
{
    QListWidgetItem *item = ui->listWidget->currentItem();
    if(item == nullptr)
    {
        cout <<"item == nullptr";
        return;
    }
    emit gotoTransfer(TransferStatus::Download);                    // 调转到下载传输列表

    DownDask *downDask = DownDask::getInstance();
    if(downDask == nullptr)
    {
        cout << "DownDask::getInstance() == nullptr";
        return;
    }

    for(auto val : m_shareFileList)
    {
        if(val->item == item)
        {
            QString  filePathName = QFileDialog::getSaveFileName(this,"选择保存文件路径", val->fileName);
            if(filePathName.isEmpty())
            {
                cout << "filePathName.isEmpty()";
                return;
            }

            int ret = downDask->appendDownloadList(val,filePathName,true);          // 添加到下载队列中
            if(ret == -1)
            {
                QMessageBox::warning(this, "下载", "任务已经在下载队列中");
            }
            else if(ret == -2)
            {
                LoginInfoInstance *login = LoginInfoInstance::getInstance(); // 获取单例
                m_common.writeRecord(login->getUser(),val->fileName, "116"); // 下载文件失败，记录
            }
            break;
        }
    }
}

void ShareFile::saveFile(FileInfo *info)
{
    LoginInfoInstance *login = LoginInfoInstance::getInstance();

    QNetworkRequest request;
    QString url = QString("http://%1:%2/sharectl?cmd=save").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray array = setDealFileJson(login->getUser(),login->getToken(),info->fileName,info->md5);

    QNetworkReply *reply = m_manger->post(request,array);

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

        QString retcode = m_common.getCode(array);
        if(retcode == "100")
        {
            QMessageBox::information(this,"转存",QString("[%1] 转存成功").arg(info->fileName));
        }
        else if(retcode == "106")
        {
            QMessageBox::information(this,"转存",QString("[%1] 文件已存在").arg(info->fileName));
        }
        else
        {
            QMessageBox::information(this,"转存",QString("[%1] 文件失败").arg(info->fileName));
        }
    });
}

void ShareFile::getFileProperty(FileInfo *info)
{
    FileProperty dlg;
    dlg.setText(info);

    dlg.exec();
}

void ShareFile::downloadFilesAction()
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
    if(downDask->isShareTask() == false)return;

     DownladoInfo *tmp = downDask->takeTask();           // 获取第一个任务

     QUrl url = tmp->url;
     QFile *file = tmp->file;
     QString md5 = tmp->md5;
     QString filename = tmp->filename;
     DataProgress *dp = tmp->dp;

     // 发送get请求
     QNetworkReply * reply = m_manger->get(QNetworkRequest(url));
     if(reply == NULL)
     {
         downDask->dealDownloadTask(); // 删除任务
         cout << "get err";
         return;
     }

     // 获取请求的数据完成时，就会发送信号SIGNAL(finished())
     connect(reply, &QNetworkReply::finished, [=]()
     {
//         if(reply->error() != QNetworkReply::NoError)
//         {
//             cout << reply->errorString();
//             reply->deleteLater();

//             downDask->dealDownloadTask();// 删除下载任务
//             QMessageBox::warning(this,"下载文件","文件下载失败");
//             return;

//         }
         cout << "下载完成";
         reply->deleteLater();
         downDask->dealDownloadTask();// 删除下载任务

         // 获取登陆信息实例
         LoginInfoInstance *login = LoginInfoInstance::getInstance(); // 获取单例
         m_common.writeRecord(login->getUser(), filename, "115"); // 下载文件成功，记录

         dealFilePv(md5, filename); // 下载文件pv字段处理
     });

     // 当有可用数据时，reply 就会发出readyRead()信号，我们这时就可以将可用的数据保存下来
     connect(reply, &QNetworkReply::readyRead, [=]()
     {
         // 如果文件存在，则写入文件
         if (file != NULL)
         {
             file->write(reply->readAll());
         }
     });

     // 有可用数据更新时
     connect(reply, &QNetworkReply::downloadProgress, [=](qint64 bytesRead, qint64 totalBytes)
     {
         dp->setProgress(bytesRead, totalBytes);// 设置进度
     });
}

void ShareFile::dealFilePv(QString md5, QString filename)
{
    LoginInfoInstance *login = LoginInfoInstance::getInstance();

    QNetworkRequest request;
    QString url = QString("http://%1:%2/sharectl?cmd=pv").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url ));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray array = setDealFileJson(login->getUser(),login->getToken(),filename,md5);

    // 发送post请求
    QNetworkReply * reply = m_manger->post(request,array);

    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError)
        {
            cout << reply->errorString();
            reply->deleteLater();
            return;
        }

        // 服务器返回用户的数据
        QByteArray array = reply->readAll();
        reply->deleteLater();

        /*
            下载文件pv字段处理
                成功：{"code":"100"}
                失败：{"code":"404"}
        */
        if(m_common.getCode(array) == "100")
        {
            for(auto val : m_shareFileList)
            {
                if(val->md5 == md5 && val->fileName == filename)
                {
                   val->pv += 1;
                   break;
                }
            }
            for(int i = 0; i < m_shareFileList.size(); ++i)
            {
                FileInfo *info = m_shareFileList.at(i);
                if( info->md5 == md5 && info->fileName == filename)
                {
                    int pv = info->pv;
                    info->pv = pv+1;
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

void ShareFile::checkTaskList()
{
    connect(&m_downloadTimer,&QTimer::timeout,this,&ShareFile::downloadFilesAction);

    m_downloadTimer.start(500);

}

void ShareFile::rightMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->listWidget->itemAt(pos);
    if(item == nullptr)
    {
        // QPoint QMouseEvent::pos()   这个只是返回相对这个widget(重载了QMouseEvent的widget)的位置。
        // QPoint QMouseEvent::globalPos()  窗口坐标，这个是返回鼠标的全局坐标
        // QPoint QCursor::pos() [static] 返回相对显示器的全局坐标
        // QWidget::pos() : QPoint 这个属性获得的是当前目前控件在父窗口中的位置
        m_menuEmpty->exec(QCursor::pos());
    }
    else
    {
        ui->listWidget->setCurrentItem(item);
        m_menuItem->exec(QCursor::pos());
    }
}
