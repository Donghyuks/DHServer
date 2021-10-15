#include "GamePlayLauncher.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QTextCodec>

GamePlayLauncher::GamePlayLauncher(QDialog*parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	codec = QTextCodec::codecForName("EUC-KR");	// �ѱ��ڵ�
	Green_Brush = new QBrush(Qt::green);
	Red_Brush = new QBrush(Qt::red);
	Bold_Rarge = new QFont("century gothic", 20, QFont::Bold);

	// Ʈ�� ����Ʈ�� Į�� ��������
	ui.treeWidget->setColumnWidth(0, 150);

	// Html ������ �о ������ (��ġ��Ʈ)
	QFile HtmlFile("PathNote.htm");
	HtmlFile.open(QFile::ReadOnly | QFile::Text);
	QTextStream HtmlStream(&HtmlFile);
	ui.textBrowser->setHtml(HtmlStream.readAll());

	// Ʈ�� ������ �߰�.
	addTreeRoot(ConvertKR("�¶���"), "", ColColorType::Green);
	addTreeRoot(ConvertKR("��������"), "", ColColorType::Red);

	addTreeChild(ConvertKR("�¶���"), ConvertKR("���α�"), ConvertKR("������"), ColColorType::Green);
	addTreeChild(ConvertKR("�¶���"), ConvertKR("�Ӱ���"), ConvertKR("�칰"), ColColorType::Green);
	addTreeChild(ConvertKR("�¶���"), ConvertKR("����������"), ConvertKR("����"), ColColorType::Green);

	addTreeChild(ConvertKR("��������"), ConvertKR("KE_KAL"), ConvertKR("������"), ColColorType::Red);
	addTreeChild(ConvertKR("��������"), ConvertKR("���˶���"), ConvertKR("�ΰ潺"), ColColorType::Red);
	addTreeChild(ConvertKR("��������"), ConvertKR("�����"), ConvertKR("������"), ColColorType::Red);
}

GamePlayLauncher::~GamePlayLauncher()
{
}

QString GamePlayLauncher::ConvertKR(QByteArray _Text)
{
	return codec->toUnicode(_Text);
}

void GamePlayLauncher::addTreeRoot(QString _Name, QString _description, ColColorType _Color_Type)
{
	QTreeWidgetItem* treeItem = new QTreeWidgetItem(ui.treeWidget);
	treeItem->setText(0, _Name);
	treeItem->setText(1, _description);
	treeItem->setFont(0, *Bold_Rarge);

	switch (_Color_Type)
	{
	case GamePlayLauncher::Green:
		treeItem->setTextColor(0, QColor(0x8a,0xe0,0x1d));
		break;
	case GamePlayLauncher::Red:
		treeItem->setTextColor(0, QColor(0xf7,0x55,0x31));
		break;
	}

	WidgetRoot_List.insert(std::pair<QString, QTreeWidgetItem*>(_Name,treeItem));
}

void GamePlayLauncher::addTreeChild(QString _Parent_Name, QString _Name, QString _description, ColColorType _Color_Type)
{
	// �ش��ϴ� �θ� ������ �����ͼ� Child�� �߰�����.
	QTreeWidgetItem* ParentWidget = WidgetRoot_List[_Parent_Name];
	QTreeWidgetItem* ChildTreeItem = new QTreeWidgetItem();
	ChildTreeItem->setText(0, _Name);
	ChildTreeItem->setText(1, _description);

	switch (_Color_Type)
	{
	case GamePlayLauncher::Green:
		ChildTreeItem->setBackground(0, *Green_Brush);
		break;
	case GamePlayLauncher::Red:
		ChildTreeItem->setBackground(0, *Red_Brush);
		break;
	}

	ParentWidget->addChild(ChildTreeItem);
}

void GamePlayLauncher::RemoveTreeChild(QString _Parent_Name, QString _Item_Name)
{
	// ������ �θ�κ��� �ڽĵ� (�࿡ �ش��ϴ� �κ�)�� ������.
	for (int i = 0; i < WidgetRoot_List[_Parent_Name]->childCount(); i++)
	{
		QString ColumnString = WidgetRoot_List[_Parent_Name]->child(i)->text(0);
		// �ش��ϴ� �ڽ� ����
		if (ColumnString == _Item_Name)
		{
			WidgetRoot_List[_Parent_Name]->removeChild(WidgetRoot_List[_Parent_Name]->child(i));
			return;
		}
	}
}
