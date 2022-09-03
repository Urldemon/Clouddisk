#include "ranking.h"
#include "ui_ranking.h"
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif
#include <QTableWidget>
#include <QNetworkReply>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

Ranking::Ranking(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Ranking)
{
    ui->setupUi(this);

    // 初始化网络链接管理
    m_manager = Common::getNetManager();

    // 获取登录信息
    m_login = LoginInfoInstance::getInstance();    // 获取单例对象

    initTableWidget();
}

Ranking::~Ranking()
{
    delete ui;
}

// 设置TableWidget表头和一些熟悉
void Ranking::initTableWidget()
{
    // 表头相关设置
    // 设置列表，3列：排名、文件名、下载量
    ui->tableWidget->setColumnCount(3);             // 设置列数
    ui->tableWidget->horizontalHeader()->setDefaultSectionSize(300);    //

    // 设置表头不可点击（默认点击后进行排列）
    ui->tableWidget->horizontalHeader()->setSectionsClickable(false);

    // 设置表头内容
    QStringList header;
    header.append("排名");
    header.append("文件名");
    header.append("下载量");
    ui->tableWidget->setHorizontalHeaderLabels(header);

    // 设置字体
    QFont font = ui->tableWidget->horizontalHeader()->font();               // 获取表头原来字体
    font.setBold(true);                                                     // 字体设置加粗
    ui->tableWidget->horizontalHeader()->setFont(font);                     // 将修改的font覆盖

    ui->tableWidget->verticalHeader()->setDefaultSectionSize(16);           // 设置处垂直方向高度
    ui->tableWidget->setFrameShape(QFrame::NoFrame);                        // 设置无边框
    ui->tableWidget->setShowGrid(false);                                    // 设置不显示格子线
    ui->tableWidget->verticalHeader()->setVisible(false);                   // 设置垂直头不可见，不自动显示行号
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);  // 单行选择
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    // 设置不可编辑
    ui->tableWidget->horizontalHeader()->resizeSection(0, 150);             // 设置表头第一列的宽度为150
    ui->tableWidget->horizontalHeader()->setFixedHeight(25);                // 设置表头的高度

    // 通过样式表，设置表头背景色
    ui->tableWidget->horizontalHeader()->setStyleSheet(
                "QHeaderView::section{"
                "background: skyblue;"
                "font: 10pt \"微软雅黑\";"
                "height: 35px;"
                "border:1px solid #c7f0ea;"
                "}");

//     ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    // 设置第0列的宽度
    ui->tableWidget->setColumnWidth(0,100);

    // 设置列宽策略，使列自适应宽度，所有列平均分来填充空白部分
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
}

void Ranking::refreshFiles()
{
    // 先获取文件个数
    m_userFilesCount = 0;

    // 获取链接对象
    LoginInfoInstance * login = LoginInfoInstance::getInstance();

    // 请求头链接
    QNetworkRequest request;
    QString url = QString("http://%1:%2/sharefile?cmd=count").arg(login->getIp()).arg(login->getPort());
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setUrl(QUrl(url));

    // 生成array数据
    QByteArray array = m_common.setGetCountjson(m_login->getUser(),m_login->getToken());

    // 发送请求信息
    QNetworkReply* reply = m_manager->post(request,array);
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
            return;
        }
        // 读取出数据
        QByteArray array = reply->readAll();
        reply->deleteLater();

        m_userFilesCount = getCountStatus(array).toInt();

        // 清空文件列表信息
        clearshareFileList();

        if(m_userFilesCount > 0)
        {
            // 说明共享列表有文件，获取文件列表
            m_start = 0;
            m_count = 10;

            // 获取文件信息列表
            getUserFileList();
        }
        else {
            //更新排行版列表
            refreshList();
        }
    });
}

void Ranking::getUserFileList()
{
    // 遍历数目，结束条件处理
    if(m_userFilesCount <= 0) // 结束条件，这个条件很重要，函数递归的结束条件
    {
         cout << "获取共享文件列表条件结束";
         // 更新排行版列表
         refreshList();

         return ;
    }
    else if(m_count > m_userFilesCount)     // 如果请求文件数量大于共享的文件数目
        m_count = m_userFilesCount;

    // 向服务器发起请求
    QNetworkRequest request;                // 创建请求对象
    QString url = QString("http://%1:%2/sharefile?cmd=pvdesc").arg(m_login->getIp()).arg(m_login->getPort());
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    // 整理好json数据
    QByteArray data = setFileListJson(m_start,m_count);

    // 改变文件起点位置
    m_start += m_count;
    m_userFilesCount -= m_count; // 文件数量递减

    // 发送post请求
    QNetworkReply *reply = m_manager->post(request,data);
    if(reply == NULL)
    {
        cout << "reply == NULL";
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

        QByteArray array = reply->readAll();
        reply->deleteLater();

        // 不是错误码处理文件列表json信息
        if(m_common.getCode(array) == "100")
        {

           getFileJsonInfo(array);

           // 继续获取共享文件列表   =====-===--==
           getUserFileList();
        }

    });
}

void Ranking::getFileJsonInfo(QByteArray data)
{

    QJsonParseError error;

    // 将获取的文件进行解析
    QJsonDocument doc = QJsonDocument::fromJson(data,&error);
    if(error.error == QJsonParseError::NoError)
    {
        if(doc.isNull() || doc.isEmpty())
        {
            cout << "doc.isNull() || doc.isEmpty()";
            return;
        }

        if(doc.isObject())
        {
            // QJsonObject json对象，描述json数据中{}括起来部分
            QJsonObject obj = doc.object();         // 获取最外层对象

            // 获取games所对应的数组
            // QJsonArray json数组，描述json数据中[]括起来部分
            QJsonArray array = obj.value("files").toArray();

            cout << array;
            long size = array.size();

            cout << "RankingFileCount:" << size;

            for(auto val:array)
            {
                // 将数组中数据转化成对象
                QJsonObject tmp = val.toObject();

                RankingFileInfo *info = new RankingFileInfo;
                info->filename = tmp.value("file_name").toString();
                info->pv = tmp.value("pv").toInt();

                // list添加结点
                m_list.push_back(info);
            }
        }
    }
    else
        cout << "err=" << error.errorString();
}

void Ranking::refreshList()
{
    int rowCount = ui->tableWidget->rowCount();         // 获取tableWidget上的行数
    for(int i = 0; i < rowCount; ++i)
    {
        // 参数为0，不是i，自动delete里面的item
        ui->tableWidget->removeRow(0);


    }

    int n = m_list.size(); // 元素个数

    rowCount = 0;
    for(int i = 0; i < n; ++i)
    {
        RankingFileInfo *tmp = m_list.at(i);
        ui->tableWidget->insertRow(rowCount);    // 插入新行

        // 新建item
        QTableWidgetItem *item1 = new QTableWidgetItem;
        QTableWidgetItem *item2 = new QTableWidgetItem;
        QTableWidgetItem *item3 = new QTableWidgetItem;

        // 设置字体显示风格
        item1->setTextAlignment(Qt::AlignHCenter |  Qt::AlignVCenter);
        item2->setTextAlignment(Qt::AlignLeft |  Qt::AlignVCenter);
        item3->setTextAlignment(Qt::AlignHCenter |  Qt::AlignVCenter);

        // 排行
        // 字体大写
        QFont font;
        font.setPointSize(10);                  // 设置字体大小
        font.setFamily("微软雅黑");              // 设置文本类型

        // 设置文字格式
        item1->setFont(font);
        item2->setFont(font);
        item3->setFont(font);

        // 排名
        item1->setText(QString::number(i+1));

        // 文件名
        item2->setText(tmp->filename);

        // 下载量
        item3->setText(QString::number(tmp->pv));

        // 设置将item整合到tableWidget上
        ui->tableWidget->setItem(rowCount, 0, item1);
        ui->tableWidget->setItem(rowCount, 1, item2);
        ui->tableWidget->setItem(rowCount, 2, item3);

        rowCount++;                              // 行++
    }
}

void Ranking::clearshareFileList()
{
    while(m_list.empty() == false)
    {
        RankingFileInfo *val = m_list.front();      // 获取头节点信息
        delete val;                                 // 释放创建的数据类型
        m_list.pop_front();                         // 删除头节点
    }
}

QByteArray Ranking::setFileListJson(int start, int count)
{
    QMap<QString,QVariant> tmp;
    tmp.insert("start",start);
    tmp.insert("count",count);
    tmp.insert("user_name",m_login->getUser());
    tmp.insert("token",m_login->getToken());

    QJsonDocument jsondocument = QJsonDocument::fromVariant(tmp);
    if(jsondocument.isNull() == true)
    {
        cout << "jsonDocument.isNull()";
        return "";
    }
    return jsondocument.toJson();
}

QString Ranking::getCountStatus(QByteArray json)
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
        if(ret == "400")
        {
            QMessageBox::warning(this,"账户异常","请重新登录！！");
            emit loginAgainSignal();
            return "";
        }
    }
    return QString(doc.object().value("count").toString());
}

