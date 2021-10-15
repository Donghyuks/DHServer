#include "GamePlayLauncher.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QTextCodec>

GamePlayLauncher::GamePlayLauncher(QDialog*parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	codec = QTextCodec::codecForName("EUC-KR");	// 한글코덱
	Green_Brush = new QBrush(Qt::green);
	Red_Brush = new QBrush(Qt::red);
	Bold_Rarge = new QFont("century gothic", 20, QFont::Bold);

	// 트리 리스트의 칼럼 간격조정
	ui.treeWidget->setColumnWidth(0, 150);

	// Html 파일을 읽어서 보여줌 (패치노트)
	QFile HtmlFile("PathNote.htm");
	HtmlFile.open(QFile::ReadOnly | QFile::Text);
	QTextStream HtmlStream(&HtmlFile);
	ui.textBrowser->setHtml(HtmlStream.readAll());

	// 트리 구조에 추가.
	addTreeRoot(ConvertKR("온라인"), "", ColColorType::Green);
	addTreeRoot(ConvertKR("오프라인"), "", ColColorType::Red);

	addTreeChild(ConvertKR("온라인"), ConvertKR("꼬부기"), ConvertKR("물대포"), ColColorType::Green);
	addTreeChild(ConvertKR("온라인"), ConvertKR("머가리"), ConvertKR("우물"), ColColorType::Green);
	addTreeChild(ConvertKR("온라인"), ConvertKR("자장면곱빼기"), ConvertKR("ㅉㅉ"), ColColorType::Green);

	addTreeChild(ConvertKR("오프라인"), ConvertKR("KE_KAL"), ConvertKR("흐정이"), ColColorType::Red);
	addTreeChild(ConvertKR("오프라인"), ConvertKR("말쫀랑득"), ConvertKR("민경스"), ColColorType::Red);
	addTreeChild(ConvertKR("오프라인"), ConvertKR("초코찌개"), ConvertKR("우진이"), ColColorType::Red);
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
	// 해당하는 부모 위젯을 가져와서 Child를 추가해줌.
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
	// 위젯의 부모로부터 자식들 (행에 해당하는 부분)을 가져옴.
	for (int i = 0; i < WidgetRoot_List[_Parent_Name]->childCount(); i++)
	{
		QString ColumnString = WidgetRoot_List[_Parent_Name]->child(i)->text(0);
		// 해당하는 자식 삭제
		if (ColumnString == _Item_Name)
		{
			WidgetRoot_List[_Parent_Name]->removeChild(WidgetRoot_List[_Parent_Name]->child(i));
			return;
		}
	}
}
