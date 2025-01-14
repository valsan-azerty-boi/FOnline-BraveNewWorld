#pragma once

//#pragma warning  (disable : 4996 34 )
//#pragma warning (disable : 4996)

#include <vcclr.h>
#include <locale.h>
#include "EditDialogNode.h"
#include "EditAnswerNode.h"
#include "EnterBox.h"
#include "MessBox.h"
#include "Defines.h"
#include "Types.h"
#include "Log.h"
#include "Dialogs.h"
#include "Vars.h"
#include "Text.h"
#include "ConstantsManager.h"
#include "FileManager.h"
#include "Version.h"

struct NpcInfo
{
	string PackName;
	string Name;
	string Avatar;
	StrVec Info[8];
	UIntStrMulMap Texts;
};
typedef vector<NpcInfo> NpcInfoVec;
NpcInfoVec AllNpcInfo;

struct DialogOpt
{
	string DlgScript;
	bool DlgNoShuffle;
	DialogOpt():DlgNoShuffle(false){}
};
typedef map<DWORD,DialogOpt> DialogOptMap;
DialogOptMap DialogOptions;


namespace DialogEditor {
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Diagnostics;

	/// <summary>
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>

	#pragma region Global definitions

	const char* ToAnsi(String^ str)
	{
        static char mb[ 0x10000 ];
        pin_ptr<const wchar_t> wch = PtrToStringChars(str);
        if( WideCharToMultiByte( CP_UTF8, 0, wch, -1, mb, 0x10000 - 1, NULL, NULL ) == 0 )
            mb[ 0 ] = 0;
		return mb;
	}

	String^ ToClrString(const char* str)
	{
        static THREAD wchar_t wc[ 0x10000 ];
        if( MultiByteToWideChar( CP_UTF8, 0, str, -1, wc, 0x10000 - 1 ) == 0 )
            wc[ 0 ] = 0;
		return String(wc).ToString();
	}

	String^ ToClrString(string& str)
	{
		return ToClrString(str.c_str());
	}

	string GetOpName(bool is_demand, char op)
	{
		static char names[64];
		switch(op)
		{
		case '>': return ">";
		case '<': return "<";
		case '=': return is_demand?"==":"=";
		case '!': return "!=";
		case '}': return ">=";
		case '{': return "<=";
		case '+': return "+";
		case '-': return "-";
		case '/': return "/";
		case '*': return "*";
		default: break;
		}
		return "error";
	}

	bool SaveTreeToBuffer(char* out_dlg);
	void RebuildDlgTree(String^ lang, DialogsVec& dlg_pack, FOMsg& texts);
	void ReloadTexts(const char* name_lang);

	void EditNode();								// Модификация узла дерева
	void MoveNode(int count);						// Перемещение узла дерева
	void ResetAll();								// Сброс всех настроек элементов формы.

	void ResetNpc();
	void ResetDialog();
	void ResetVar();

	void UpdateAll();

	void UpdateVarsList();
	void UpdateDlgList();

	void UpdateDialog();
	void UpdateDlgTree();

	bool SaveProtoToBuffer(char * out_dlg);
	System::Void UpdateListBoxes();
	String^ GetParamDesc(String^ str);

	TemplateVar* curVar;

#pragma endregion	
	public ref class Form1 : public System::Windows::Forms::Form
	{
	protected:
		~Form1(void)
		{
			ResetAll();
			if(components)
			{
//				delete components;
//				delete txtTmp;
//				delete txtTmp2;
//				delete cbTmp;
			}
		//	SAFEDEL_(edCrit);
		//	SAFEDEL_(gvar_list);
		//	SAFEDEL_(pvar_list);
		//	SAFEDEL_(lvar_list);
		//	SAFEDEL(var_chg);
		}
	private: System::Windows::Forms::TabPage^  tabInfo;
	protected: 
	private: System::Windows::Forms::TextBox^  tInfoAvatar;
	private: System::Windows::Forms::Label^  lInfoAvatar;
	private: System::Windows::Forms::TextBox^  tInfoName;
	private: System::Windows::Forms::GroupBox^  groupBox1;
	private: System::Windows::Forms::Label^  lInfoNum;
	private: System::Windows::Forms::Label^  lInfoAbout;
	private: System::Windows::Forms::Label^  lInfoFull;
	private: System::Windows::Forms::Label^  lInfoShort;
	private: System::Windows::Forms::NumericUpDown^  numInfoNum;
	private: System::Windows::Forms::RichTextBox^  rtInfoFull;
	private: System::Windows::Forms::RichTextBox^  rtInfoShort;
	private: System::Windows::Forms::RadioButton^  rbInfoCriticalDead;
	private: System::Windows::Forms::RadioButton^  rbInfoDead;
	private: System::Windows::Forms::RadioButton^  rbInfoKnockout;
	private: System::Windows::Forms::RadioButton^  rbInfoLive;
	private: System::Windows::Forms::Label^  lInfoName;
	private: System::Windows::Forms::TabPage^  tabComment;
	private: System::Windows::Forms::RichTextBox^  rtComment;
	private: System::Windows::Forms::Button^  btnDialogAddTree;
	private: System::Windows::Forms::TabPage^  tabText;
	private: System::Windows::Forms::DataGridView^  gridText;
	private: System::Windows::Forms::Button^  btnTreeToMain;
	private: System::Windows::Forms::CheckBox^  cbVarNoCheck;
	private: System::Windows::Forms::CheckBox^  cbDialogNoShuffle;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  gNumber;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  gText;
	private: System::Windows::Forms::GroupBox^  gbDemandScript;
	private: System::Windows::Forms::RadioButton^  rbDemandScriptVal5;
	private: System::Windows::Forms::RadioButton^  rbDemandScriptVal4;
	private: System::Windows::Forms::RadioButton^  rbDemandScriptVal3;
	private: System::Windows::Forms::RadioButton^  rbDemandScriptVal2;
	private: System::Windows::Forms::RadioButton^  rbDemandScriptVal1;
	private: System::Windows::Forms::RadioButton^  rbDemandScriptVal0;
	private: System::Windows::Forms::GroupBox^  gbResultScript;
	private: System::Windows::Forms::RadioButton^  rbResultScriptVal0;
	private: System::Windows::Forms::RadioButton^  rbResultScriptVal5;
	private: System::Windows::Forms::RadioButton^  rbResultScriptVal4;
	private: System::Windows::Forms::RadioButton^  rbResultScriptVal3;
	private: System::Windows::Forms::RadioButton^  rbResultScriptVal2;
	private: System::Windows::Forms::RadioButton^  rbResultScriptVal1;
	private: System::Windows::Forms::ComboBox^  cbDemandSValue4;
	private: System::Windows::Forms::ComboBox^  cbDemandSValue3;
	private: System::Windows::Forms::ComboBox^  cbDemandSValue2;
	private: System::Windows::Forms::ComboBox^  cbDemandSValue1;
	private: System::Windows::Forms::ComboBox^  cbDemandSValue0;
	private: System::Windows::Forms::ComboBox^  cbResultSValue4;
	private: System::Windows::Forms::ComboBox^  cbResultSValue3;
	private: System::Windows::Forms::ComboBox^  cbResultSValue2;
	private: System::Windows::Forms::ComboBox^  cbResultSValue1;
	private: System::Windows::Forms::ComboBox^  cbResultSValue0;
	private: System::Windows::Forms::CheckBox^  cbDemandNoRecheck;
	private: System::Windows::Forms::Button^  btnDemandOr;
	private: System::Windows::Forms::ComboBox^  cbDemandValue;
	private: System::Windows::Forms::ComboBox^  cbResultValue;
	private: System::Windows::Forms::RadioButton^  rbVarLocalItem;
	private: System::Windows::Forms::RadioButton^  rbVarLocalMap;
	private: System::Windows::Forms::RadioButton^  rbVarLocalLocation;
	public: 
		int Lang;

		Form1(void)
		{
			InitializeComponent();
			curVar=NULL;
		}

	private: cli::array<String^>^ ParamsName;
	private: cli::array<String^>^ ParamsDesc;
	private: cli::array<TreeView^>^ LangTree;

#pragma region Elements of Form
//------------Elements of Form--------------------------------------------------
//	private: System::Windows::Forms::SaveFileDialog^	dlg_save;
//	private: System::Windows::Forms::OpenFileDialog^	dlg_open;

	private: Edit_node	enD;
	private: Edit_nodeA enA;
	private: EnterBox	EntBox;
//------------------------------------------------------------------------------
#pragma endregion

private: System::ComponentModel::IContainer^  components;
private: String^ path_data;				// Путь к папке data
private: String^ path_npc;				// Путь к папке npc
//private: String^ path_db;				// Путь к папке CritProtoDB
//private: String^ path_patch;			// Путь к папке Patch

#pragma region Form_s elements

private: System::Windows::Forms::StatusStrip^  statusStrip1;
private: System::Windows::Forms::ToolStripStatusLabel^  lSSLabel1;
private: System::Windows::Forms::ToolStripStatusLabel^  lErr;
private: System::Windows::Forms::TabControl^  tabControl1;
private: System::Windows::Forms::TabPage^  tabDialogs;
private: System::Windows::Forms::TabPage^  tabVars;

private: System::Windows::Forms::RadioButton^  radioButton1;
private: System::Windows::Forms::RadioButton^  btnSkill;
private: System::Windows::Forms::RadioButton^  btnStat;
private: System::Windows::Forms::RadioButton^  localBtn;
private: System::Windows::Forms::RadioButton^  playerBtn;
private: System::Windows::Forms::RadioButton^  globalBtn;
private: System::Windows::Forms::GroupBox^  groupBox3;
private: System::Windows::Forms::NumericUpDown^  numVarMax;
private: System::Windows::Forms::NumericUpDown^  numVarId;
private: System::Windows::Forms::NumericUpDown^  numVarMin;
private: System::Windows::Forms::RichTextBox^  txtVarDesc;

private: System::Windows::Forms::NumericUpDown^  numVarStart;
private: System::Windows::Forms::TextBox^  txtVarName;
private: System::Windows::Forms::Label^  label20;
private: System::Windows::Forms::Label^  label23;
private: System::Windows::Forms::Label^  label25;
private: System::Windows::Forms::Label^  label37;
private: System::Windows::Forms::Label^  label38;
private: System::Windows::Forms::Button^  btnVarChange;
private: System::Windows::Forms::Button^  btnVarDelete;
private: System::Windows::Forms::Button^  btnVarAdd;
private: System::Windows::Forms::Label^  label31;
private: System::Windows::Forms::Label^  label29;
private: System::Windows::Forms::Label^  label33;
private: System::Windows::Forms::Label^  lbIntro;
private: System::Windows::Forms::Label^  label34;
private: System::Windows::Forms::GroupBox^  groupBox8;
private: System::Windows::Forms::TabControl^  tabControl2;
private: System::Windows::Forms::TabPage^  tabDialog;
private: System::Windows::Forms::TabPage^  tabAnswer;
private: System::Windows::Forms::TabPage^  tabDemand;
private: System::Windows::Forms::TabPage^  tabResult;
private: System::Windows::Forms::Label^  label26;


private: System::Windows::Forms::NumericUpDown^  numIDdialog;
private: System::Windows::Forms::CheckBox^  cbAID;
private: System::Windows::Forms::Label^  label1;
private: System::Windows::Forms::Button^  btnAddDialog;

private: System::Windows::Forms::RichTextBox^  txtDialog;
private: System::Windows::Forms::ComboBox^  cbAction_no_answ;
private: System::Windows::Forms::Label^  label39;
private: System::Windows::Forms::RichTextBox^  txtAnswer;
private: System::Windows::Forms::NumericUpDown^  numLink;

private: System::Windows::Forms::Button^  btnAddAnswer;

private: System::Windows::Forms::Button^  btnUpLine;
private: System::Windows::Forms::Button^  btnDownLine;


private: System::Windows::Forms::Button^  btnDeleteLine;
private: System::Windows::Forms::Label^  label40;
private: System::Windows::Forms::GroupBox^  groupBox2;
private: System::Windows::Forms::ListBox^  lbAllVars;
private: System::Windows::Forms::ListBox^  lbVars;
private: System::Windows::Forms::TabPage^  tabDialogsMenu;
private: System::Windows::Forms::Button^  btnUslRezDelete;
private: System::Windows::Forms::RadioButton^  rbRez;
private: System::Windows::Forms::RadioButton^  rbUsl;
private: System::Windows::Forms::Button^  btnUslRezChange;
private: System::Windows::Forms::Button^  btnUslRezAdd;
private: System::Windows::Forms::Label^  label22;
private: System::Windows::Forms::ComboBox^  cbVarType;
private: System::Windows::Forms::ComboBox^  cbVar_usl;
private: System::Windows::Forms::ComboBox^  cbStat_usl;
private: System::Windows::Forms::ComboBox^  cbPerk_usl;
private: System::Windows::Forms::ComboBox^  cbSkill_usl;
private: System::Windows::Forms::RadioButton^  rbVar;
private: System::Windows::Forms::RadioButton^  rbStat;
private: System::Windows::Forms::RadioButton^  rbPerk;
private: System::Windows::Forms::RadioButton^  rbSkill;
private: System::Windows::Forms::Button^  btnUslRez;
private: System::Windows::Forms::Label^  label28;
private: System::Windows::Forms::NumericUpDown^  numLock;
private: System::Windows::Forms::RadioButton^  rbLock;
private: System::Windows::Forms::ComboBox^  cbQuest_usl;
private: System::Windows::Forms::Label^  label21;
private: System::Windows::Forms::ComboBox^  cbQuest_rez;
private: System::Windows::Forms::NumericUpDown^  numUslRezValue;
private: System::Windows::Forms::ComboBox^  cbItem_usl;
private: System::Windows::Forms::NumericUpDown^  numItem;
private: System::Windows::Forms::RadioButton^  rbQuest;
private: System::Windows::Forms::ComboBox^  cbPlayerType;
private: System::Windows::Forms::RadioButton^  rbItem;
private: System::Windows::Forms::NumericUpDown^  numQuest;
private: System::Windows::Forms::Label^  label19;
private: System::Windows::Forms::ComboBox^  cbDemandVars;
private: System::Windows::Forms::ComboBox^  cbDemandParam;
private: System::Windows::Forms::RadioButton^  rbDemandVar;
private: System::Windows::Forms::RadioButton^  rbDemandParam;
private: System::Windows::Forms::Button^  btnDemandOper;

private: System::Windows::Forms::RadioButton^  rbDemandItem;
private: System::Windows::Forms::Label^  label43;
private: System::Windows::Forms::ComboBox^  cbResultVars;
private: System::Windows::Forms::ComboBox^  cbResultParam;
private: System::Windows::Forms::RadioButton^  rbResultVar;
private: System::Windows::Forms::RadioButton^  rbResultParam;
private: System::Windows::Forms::Button^  btnResultOper;

private: System::Windows::Forms::RadioButton^  rbResultItem;
private: System::Windows::Forms::Label^  label41;
private: System::Windows::Forms::NumericUpDown^  numResultLock;
private: System::Windows::Forms::RadioButton^  radioButton8;
private: System::Windows::Forms::Button^  btnDialogCheck;
private: System::Windows::Forms::Button^  btnDialogClear;
private: System::Windows::Forms::Button^  btnDemandAdd;
private: System::Windows::Forms::Button^  btnResultAdd;
private: System::Windows::Forms::Label^  lErrVarCount;
private: System::Windows::Forms::Label^  label30;
private: System::Windows::Forms::Label^  lErrCount;
private: System::Windows::Forms::Label^  label27;
private: System::Windows::Forms::GroupBox^  gbDialogsDB;
private: System::Windows::Forms::Button^  btnDialogErase;
private: System::Windows::Forms::NumericUpDown^  numDialogsId;
private: System::Windows::Forms::TextBox^  txtDialogName;
private: System::Windows::Forms::Label^  label36;
private: System::Windows::Forms::Label^  label35;
private: System::Windows::Forms::ListBox^  lbDialogsList;
private: System::Windows::Forms::Button^  btnDialogSave;
private: System::Windows::Forms::Button^  btnDlgSaveAs;
private: System::Windows::Forms::Button^  btnDlgLoad;
private: System::Windows::Forms::GroupBox^  gbDialogsTexts;
private: System::Windows::Forms::Button^  btnTextCheck;
private: System::Windows::Forms::Button^  btnReloadTexts;
private: System::Windows::Forms::TextBox^  txtDialogsLang;
private: System::Windows::Forms::Label^  label42;
private: System::Windows::Forms::Panel^  panel1;
private: System::Windows::Forms::RadioButton^  rbResultNpc;
private: System::Windows::Forms::RadioButton^  rbResultPlayer;
private: System::Windows::Forms::Label^  label62;
private: System::Windows::Forms::Panel^  panel2;
private: System::Windows::Forms::Label^  label63;
private: System::Windows::Forms::RadioButton^  rbDemandNpc;
private: System::Windows::Forms::RadioButton^  rbDemandPlayer;
private: System::Windows::Forms::TextBox^  txtDemandScript;
private: System::Windows::Forms::RadioButton^  rbDemandScript;
private: System::Windows::Forms::TextBox^  txtResultScript;
private: System::Windows::Forms::RadioButton^  rbResultScript;
private: System::Windows::Forms::RadioButton^  rbVarUnicum;
private: System::Windows::Forms::RadioButton^  rbVarLocal;
private: System::Windows::Forms::RadioButton^  rbVarGlobal;
private: System::Windows::Forms::ComboBox^  cbLinkOther;
private: System::Windows::Forms::RadioButton^  rbLinkOther;
private: System::Windows::Forms::RadioButton^  rbLinkId;
private: System::Windows::Forms::CheckBox^  cbVarRandom;
private: System::Windows::Forms::CheckBox^  cbVarQuest;
private: System::Windows::Forms::ComboBox^  cbDemandItem;
private: System::Windows::Forms::ComboBox^  cbResultItem;
private: System::Windows::Forms::Button^  btnDlgSave;
private: System::Windows::Forms::Label^  lDlgName;
private: System::Windows::Forms::Label^  label24;
private: System::Windows::Forms::TabControl^  tcLang;
private: System::Windows::Forms::TabPage^  tbMain;
private: System::Windows::Forms::RadioButton^  rbEnglish;
private: System::Windows::Forms::RadioButton^  rbRussian;
private: System::Windows::Forms::Label^  label64;
private: System::Windows::Forms::ToolTip^  toolTip1;
#pragma endregion

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
	private:
		void InitializeComponent(void)
		{
            this->components = (gcnew System::ComponentModel::Container());
            System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle1 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
            System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(Form1::typeid));
            this->toolTip1 = (gcnew System::Windows::Forms::ToolTip(this->components));
            this->statusStrip1 = (gcnew System::Windows::Forms::StatusStrip());
            this->lSSLabel1 = (gcnew System::Windows::Forms::ToolStripStatusLabel());
            this->lErr = (gcnew System::Windows::Forms::ToolStripStatusLabel());
            this->tabControl1 = (gcnew System::Windows::Forms::TabControl());
            this->tabDialogs = (gcnew System::Windows::Forms::TabPage());
            this->tcLang = (gcnew System::Windows::Forms::TabControl());
            this->tabControl2 = (gcnew System::Windows::Forms::TabControl());
            this->tabDialogsMenu = (gcnew System::Windows::Forms::TabPage());
            this->btnTreeToMain = (gcnew System::Windows::Forms::Button());
            this->btnDialogAddTree = (gcnew System::Windows::Forms::Button());
            this->lDlgName = (gcnew System::Windows::Forms::Label());
            this->label24 = (gcnew System::Windows::Forms::Label());
            this->btnDlgSave = (gcnew System::Windows::Forms::Button());
            this->gbDialogsTexts = (gcnew System::Windows::Forms::GroupBox());
            this->btnTextCheck = (gcnew System::Windows::Forms::Button());
            this->btnReloadTexts = (gcnew System::Windows::Forms::Button());
            this->txtDialogsLang = (gcnew System::Windows::Forms::TextBox());
            this->label42 = (gcnew System::Windows::Forms::Label());
            this->btnDlgSaveAs = (gcnew System::Windows::Forms::Button());
            this->btnDlgLoad = (gcnew System::Windows::Forms::Button());
            this->gbDialogsDB = (gcnew System::Windows::Forms::GroupBox());
            this->btnDialogErase = (gcnew System::Windows::Forms::Button());
            this->numDialogsId = (gcnew System::Windows::Forms::NumericUpDown());
            this->txtDialogName = (gcnew System::Windows::Forms::TextBox());
            this->label36 = (gcnew System::Windows::Forms::Label());
            this->label35 = (gcnew System::Windows::Forms::Label());
            this->lbDialogsList = (gcnew System::Windows::Forms::ListBox());
            this->btnDialogSave = (gcnew System::Windows::Forms::Button());
            this->lErrVarCount = (gcnew System::Windows::Forms::Label());
            this->label30 = (gcnew System::Windows::Forms::Label());
            this->lErrCount = (gcnew System::Windows::Forms::Label());
            this->label27 = (gcnew System::Windows::Forms::Label());
            this->btnDialogCheck = (gcnew System::Windows::Forms::Button());
            this->btnDialogClear = (gcnew System::Windows::Forms::Button());
            this->tabInfo = (gcnew System::Windows::Forms::TabPage());
            this->tInfoAvatar = (gcnew System::Windows::Forms::TextBox());
            this->lInfoAvatar = (gcnew System::Windows::Forms::Label());
            this->tInfoName = (gcnew System::Windows::Forms::TextBox());
            this->groupBox1 = (gcnew System::Windows::Forms::GroupBox());
            this->lInfoNum = (gcnew System::Windows::Forms::Label());
            this->lInfoAbout = (gcnew System::Windows::Forms::Label());
            this->lInfoFull = (gcnew System::Windows::Forms::Label());
            this->lInfoShort = (gcnew System::Windows::Forms::Label());
            this->numInfoNum = (gcnew System::Windows::Forms::NumericUpDown());
            this->rtInfoFull = (gcnew System::Windows::Forms::RichTextBox());
            this->rtInfoShort = (gcnew System::Windows::Forms::RichTextBox());
            this->rbInfoCriticalDead = (gcnew System::Windows::Forms::RadioButton());
            this->rbInfoDead = (gcnew System::Windows::Forms::RadioButton());
            this->rbInfoKnockout = (gcnew System::Windows::Forms::RadioButton());
            this->rbInfoLive = (gcnew System::Windows::Forms::RadioButton());
            this->lInfoName = (gcnew System::Windows::Forms::Label());
            this->tabDialog = (gcnew System::Windows::Forms::TabPage());
            this->cbDialogNoShuffle = (gcnew System::Windows::Forms::CheckBox());
            this->label40 = (gcnew System::Windows::Forms::Label());
            this->label26 = (gcnew System::Windows::Forms::Label());
            this->numIDdialog = (gcnew System::Windows::Forms::NumericUpDown());
            this->cbAID = (gcnew System::Windows::Forms::CheckBox());
            this->label1 = (gcnew System::Windows::Forms::Label());
            this->btnAddDialog = (gcnew System::Windows::Forms::Button());
            this->txtDialog = (gcnew System::Windows::Forms::RichTextBox());
            this->cbAction_no_answ = (gcnew System::Windows::Forms::ComboBox());
            this->tabAnswer = (gcnew System::Windows::Forms::TabPage());
            this->cbLinkOther = (gcnew System::Windows::Forms::ComboBox());
            this->rbLinkOther = (gcnew System::Windows::Forms::RadioButton());
            this->rbLinkId = (gcnew System::Windows::Forms::RadioButton());
            this->label39 = (gcnew System::Windows::Forms::Label());
            this->txtAnswer = (gcnew System::Windows::Forms::RichTextBox());
            this->numLink = (gcnew System::Windows::Forms::NumericUpDown());
            this->btnAddAnswer = (gcnew System::Windows::Forms::Button());
            this->tabDemand = (gcnew System::Windows::Forms::TabPage());
            this->cbDemandValue = (gcnew System::Windows::Forms::ComboBox());
            this->btnDemandOr = (gcnew System::Windows::Forms::Button());
            this->cbDemandNoRecheck = (gcnew System::Windows::Forms::CheckBox());
            this->gbDemandScript = (gcnew System::Windows::Forms::GroupBox());
            this->cbDemandSValue4 = (gcnew System::Windows::Forms::ComboBox());
            this->cbDemandSValue3 = (gcnew System::Windows::Forms::ComboBox());
            this->cbDemandSValue2 = (gcnew System::Windows::Forms::ComboBox());
            this->cbDemandSValue1 = (gcnew System::Windows::Forms::ComboBox());
            this->cbDemandSValue0 = (gcnew System::Windows::Forms::ComboBox());
            this->rbDemandScriptVal0 = (gcnew System::Windows::Forms::RadioButton());
            this->rbDemandScriptVal5 = (gcnew System::Windows::Forms::RadioButton());
            this->rbDemandScriptVal4 = (gcnew System::Windows::Forms::RadioButton());
            this->rbDemandScriptVal3 = (gcnew System::Windows::Forms::RadioButton());
            this->rbDemandScriptVal2 = (gcnew System::Windows::Forms::RadioButton());
            this->rbDemandScriptVal1 = (gcnew System::Windows::Forms::RadioButton());
            this->cbDemandItem = (gcnew System::Windows::Forms::ComboBox());
            this->txtDemandScript = (gcnew System::Windows::Forms::TextBox());
            this->rbDemandScript = (gcnew System::Windows::Forms::RadioButton());
            this->panel2 = (gcnew System::Windows::Forms::Panel());
            this->label63 = (gcnew System::Windows::Forms::Label());
            this->rbDemandNpc = (gcnew System::Windows::Forms::RadioButton());
            this->rbDemandPlayer = (gcnew System::Windows::Forms::RadioButton());
            this->btnDemandAdd = (gcnew System::Windows::Forms::Button());
            this->label19 = (gcnew System::Windows::Forms::Label());
            this->cbDemandVars = (gcnew System::Windows::Forms::ComboBox());
            this->cbDemandParam = (gcnew System::Windows::Forms::ComboBox());
            this->rbDemandVar = (gcnew System::Windows::Forms::RadioButton());
            this->rbDemandParam = (gcnew System::Windows::Forms::RadioButton());
            this->btnDemandOper = (gcnew System::Windows::Forms::Button());
            this->rbDemandItem = (gcnew System::Windows::Forms::RadioButton());
            this->tabResult = (gcnew System::Windows::Forms::TabPage());
            this->cbResultValue = (gcnew System::Windows::Forms::ComboBox());
            this->gbResultScript = (gcnew System::Windows::Forms::GroupBox());
            this->cbResultSValue4 = (gcnew System::Windows::Forms::ComboBox());
            this->cbResultSValue3 = (gcnew System::Windows::Forms::ComboBox());
            this->cbResultSValue2 = (gcnew System::Windows::Forms::ComboBox());
            this->cbResultSValue1 = (gcnew System::Windows::Forms::ComboBox());
            this->cbResultSValue0 = (gcnew System::Windows::Forms::ComboBox());
            this->rbResultScriptVal0 = (gcnew System::Windows::Forms::RadioButton());
            this->rbResultScriptVal5 = (gcnew System::Windows::Forms::RadioButton());
            this->rbResultScriptVal4 = (gcnew System::Windows::Forms::RadioButton());
            this->rbResultScriptVal3 = (gcnew System::Windows::Forms::RadioButton());
            this->rbResultScriptVal2 = (gcnew System::Windows::Forms::RadioButton());
            this->rbResultScriptVal1 = (gcnew System::Windows::Forms::RadioButton());
            this->cbResultItem = (gcnew System::Windows::Forms::ComboBox());
            this->txtResultScript = (gcnew System::Windows::Forms::TextBox());
            this->rbResultScript = (gcnew System::Windows::Forms::RadioButton());
            this->panel1 = (gcnew System::Windows::Forms::Panel());
            this->label62 = (gcnew System::Windows::Forms::Label());
            this->rbResultNpc = (gcnew System::Windows::Forms::RadioButton());
            this->rbResultPlayer = (gcnew System::Windows::Forms::RadioButton());
            this->btnResultAdd = (gcnew System::Windows::Forms::Button());
            this->label41 = (gcnew System::Windows::Forms::Label());
            this->numResultLock = (gcnew System::Windows::Forms::NumericUpDown());
            this->radioButton8 = (gcnew System::Windows::Forms::RadioButton());
            this->label43 = (gcnew System::Windows::Forms::Label());
            this->cbResultVars = (gcnew System::Windows::Forms::ComboBox());
            this->cbResultParam = (gcnew System::Windows::Forms::ComboBox());
            this->rbResultVar = (gcnew System::Windows::Forms::RadioButton());
            this->rbResultParam = (gcnew System::Windows::Forms::RadioButton());
            this->btnResultOper = (gcnew System::Windows::Forms::Button());
            this->rbResultItem = (gcnew System::Windows::Forms::RadioButton());
            this->tabText = (gcnew System::Windows::Forms::TabPage());
            this->gridText = (gcnew System::Windows::Forms::DataGridView());
            this->gNumber = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
            this->gText = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
            this->tabComment = (gcnew System::Windows::Forms::TabPage());
            this->rtComment = (gcnew System::Windows::Forms::RichTextBox());
            this->tabVars = (gcnew System::Windows::Forms::TabPage());
            this->btnVarDelete = (gcnew System::Windows::Forms::Button());
            this->btnVarAdd = (gcnew System::Windows::Forms::Button());
            this->btnVarChange = (gcnew System::Windows::Forms::Button());
            this->groupBox2 = (gcnew System::Windows::Forms::GroupBox());
            this->lbAllVars = (gcnew System::Windows::Forms::ListBox());
            this->groupBox8 = (gcnew System::Windows::Forms::GroupBox());
            this->rbVarLocalItem = (gcnew System::Windows::Forms::RadioButton());
            this->cbVarNoCheck = (gcnew System::Windows::Forms::CheckBox());
            this->rbVarLocalMap = (gcnew System::Windows::Forms::RadioButton());
            this->rbVarLocalLocation = (gcnew System::Windows::Forms::RadioButton());
            this->cbVarRandom = (gcnew System::Windows::Forms::CheckBox());
            this->cbVarQuest = (gcnew System::Windows::Forms::CheckBox());
            this->rbVarUnicum = (gcnew System::Windows::Forms::RadioButton());
            this->rbVarGlobal = (gcnew System::Windows::Forms::RadioButton());
            this->rbVarLocal = (gcnew System::Windows::Forms::RadioButton());
            this->groupBox3 = (gcnew System::Windows::Forms::GroupBox());
            this->numVarMax = (gcnew System::Windows::Forms::NumericUpDown());
            this->numVarId = (gcnew System::Windows::Forms::NumericUpDown());
            this->numVarMin = (gcnew System::Windows::Forms::NumericUpDown());
            this->txtVarDesc = (gcnew System::Windows::Forms::RichTextBox());
            this->numVarStart = (gcnew System::Windows::Forms::NumericUpDown());
            this->txtVarName = (gcnew System::Windows::Forms::TextBox());
            this->label20 = (gcnew System::Windows::Forms::Label());
            this->label23 = (gcnew System::Windows::Forms::Label());
            this->label25 = (gcnew System::Windows::Forms::Label());
            this->label37 = (gcnew System::Windows::Forms::Label());
            this->label38 = (gcnew System::Windows::Forms::Label());
            this->tbMain = (gcnew System::Windows::Forms::TabPage());
            this->rbEnglish = (gcnew System::Windows::Forms::RadioButton());
            this->rbRussian = (gcnew System::Windows::Forms::RadioButton());
            this->label64 = (gcnew System::Windows::Forms::Label());
            this->btnUpLine = (gcnew System::Windows::Forms::Button());
            this->btnDeleteLine = (gcnew System::Windows::Forms::Button());
            this->btnDownLine = (gcnew System::Windows::Forms::Button());
            this->radioButton1 = (gcnew System::Windows::Forms::RadioButton());
            this->btnSkill = (gcnew System::Windows::Forms::RadioButton());
            this->btnStat = (gcnew System::Windows::Forms::RadioButton());
            this->localBtn = (gcnew System::Windows::Forms::RadioButton());
            this->playerBtn = (gcnew System::Windows::Forms::RadioButton());
            this->globalBtn = (gcnew System::Windows::Forms::RadioButton());
            this->label31 = (gcnew System::Windows::Forms::Label());
            this->label29 = (gcnew System::Windows::Forms::Label());
            this->label33 = (gcnew System::Windows::Forms::Label());
            this->lbIntro = (gcnew System::Windows::Forms::Label());
            this->label34 = (gcnew System::Windows::Forms::Label());
            this->lbVars = (gcnew System::Windows::Forms::ListBox());
            this->btnUslRezDelete = (gcnew System::Windows::Forms::Button());
            this->rbRez = (gcnew System::Windows::Forms::RadioButton());
            this->rbUsl = (gcnew System::Windows::Forms::RadioButton());
            this->btnUslRezChange = (gcnew System::Windows::Forms::Button());
            this->btnUslRezAdd = (gcnew System::Windows::Forms::Button());
            this->label22 = (gcnew System::Windows::Forms::Label());
            this->cbVarType = (gcnew System::Windows::Forms::ComboBox());
            this->cbVar_usl = (gcnew System::Windows::Forms::ComboBox());
            this->cbStat_usl = (gcnew System::Windows::Forms::ComboBox());
            this->cbPerk_usl = (gcnew System::Windows::Forms::ComboBox());
            this->cbSkill_usl = (gcnew System::Windows::Forms::ComboBox());
            this->rbVar = (gcnew System::Windows::Forms::RadioButton());
            this->rbStat = (gcnew System::Windows::Forms::RadioButton());
            this->rbPerk = (gcnew System::Windows::Forms::RadioButton());
            this->rbSkill = (gcnew System::Windows::Forms::RadioButton());
            this->btnUslRez = (gcnew System::Windows::Forms::Button());
            this->label28 = (gcnew System::Windows::Forms::Label());
            this->numLock = (gcnew System::Windows::Forms::NumericUpDown());
            this->rbLock = (gcnew System::Windows::Forms::RadioButton());
            this->cbQuest_usl = (gcnew System::Windows::Forms::ComboBox());
            this->label21 = (gcnew System::Windows::Forms::Label());
            this->cbQuest_rez = (gcnew System::Windows::Forms::ComboBox());
            this->numUslRezValue = (gcnew System::Windows::Forms::NumericUpDown());
            this->cbItem_usl = (gcnew System::Windows::Forms::ComboBox());
            this->numItem = (gcnew System::Windows::Forms::NumericUpDown());
            this->rbQuest = (gcnew System::Windows::Forms::RadioButton());
            this->cbPlayerType = (gcnew System::Windows::Forms::ComboBox());
            this->rbItem = (gcnew System::Windows::Forms::RadioButton());
            this->numQuest = (gcnew System::Windows::Forms::NumericUpDown());
            this->statusStrip1->SuspendLayout();
            this->tabControl1->SuspendLayout();
            this->tabDialogs->SuspendLayout();
            this->tabControl2->SuspendLayout();
            this->tabDialogsMenu->SuspendLayout();
            this->gbDialogsTexts->SuspendLayout();
            this->gbDialogsDB->SuspendLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numDialogsId))->BeginInit();
            this->tabInfo->SuspendLayout();
            this->groupBox1->SuspendLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numInfoNum))->BeginInit();
            this->tabDialog->SuspendLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numIDdialog))->BeginInit();
            this->tabAnswer->SuspendLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numLink))->BeginInit();
            this->tabDemand->SuspendLayout();
            this->gbDemandScript->SuspendLayout();
            this->panel2->SuspendLayout();
            this->tabResult->SuspendLayout();
            this->gbResultScript->SuspendLayout();
            this->panel1->SuspendLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numResultLock))->BeginInit();
            this->tabText->SuspendLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->gridText))->BeginInit();
            this->tabComment->SuspendLayout();
            this->tabVars->SuspendLayout();
            this->groupBox2->SuspendLayout();
            this->groupBox8->SuspendLayout();
            this->groupBox3->SuspendLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numVarMax))->BeginInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numVarId))->BeginInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numVarMin))->BeginInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numVarStart))->BeginInit();
            this->tbMain->SuspendLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numLock))->BeginInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numUslRezValue))->BeginInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numItem))->BeginInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numQuest))->BeginInit();
            this->SuspendLayout();
            // 
            // toolTip1
            // 
            this->toolTip1->IsBalloon = true;
            this->toolTip1->StripAmpersands = true;
            // 
            // statusStrip1
            // 
            this->statusStrip1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(2) {this->lSSLabel1, this->lErr});
            this->statusStrip1->Location = System::Drawing::Point(0, 448);
            this->statusStrip1->Name = L"statusStrip1";
            this->statusStrip1->Size = System::Drawing::Size(692, 22);
            this->statusStrip1->TabIndex = 8;
            this->statusStrip1->Text = L"statusStrip1";
            // 
            // lSSLabel1
            // 
            this->lSSLabel1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.25F, System::Drawing::FontStyle::Bold));
            this->lSSLabel1->ForeColor = System::Drawing::Color::Green;
            this->lSSLabel1->Name = L"lSSLabel1";
            this->lSSLabel1->Size = System::Drawing::Size(124, 17);
            this->lSSLabel1->Text = L"Последняя ошибка:";
            // 
            // lErr
            // 
            this->lErr->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(192)), static_cast<System::Int32>(static_cast<System::Byte>(0)), 
                static_cast<System::Int32>(static_cast<System::Byte>(0)));
            this->lErr->Name = L"lErr";
            this->lErr->Size = System::Drawing::Size(22, 17);
            this->lErr->Text = L"---";
            // 
            // tabControl1
            // 
            this->tabControl1->Appearance = System::Windows::Forms::TabAppearance::FlatButtons;
            this->tabControl1->Controls->Add(this->tabDialogs);
            this->tabControl1->Controls->Add(this->tabVars);
            this->tabControl1->Controls->Add(this->tbMain);
            this->tabControl1->Dock = System::Windows::Forms::DockStyle::Fill;
            this->tabControl1->Location = System::Drawing::Point(0, 0);
            this->tabControl1->Name = L"tabControl1";
            this->tabControl1->RightToLeft = System::Windows::Forms::RightToLeft::No;
            this->tabControl1->SelectedIndex = 7;
            this->tabControl1->Size = System::Drawing::Size(692, 470);
            this->tabControl1->TabIndex = 1;
            this->tabControl1->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::tabControl1_SelectedIndexChanged);
            // 
            // tabDialogs
            // 
            this->tabDialogs->Controls->Add(this->tcLang);
            this->tabDialogs->Controls->Add(this->tabControl2);
            this->tabDialogs->Location = System::Drawing::Point(4, 25);
            this->tabDialogs->Name = L"tabDialogs";
            this->tabDialogs->Size = System::Drawing::Size(684, 441);
            this->tabDialogs->TabIndex = 11;
            this->tabDialogs->Text = L"Редактор диалогов";
            this->tabDialogs->UseVisualStyleBackColor = true;
            // 
            // tcLang
            // 
            this->tcLang->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
                | System::Windows::Forms::AnchorStyles::Left) 
                | System::Windows::Forms::AnchorStyles::Right));
            this->tcLang->Location = System::Drawing::Point(0, 3);
            this->tcLang->Name = L"tcLang";
            this->tcLang->SelectedIndex = 0;
            this->tcLang->Size = System::Drawing::Size(684, 207);
            this->tcLang->TabIndex = 7;
            this->tcLang->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::tcLang_SelectedIndexChanged);
            this->tcLang->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &Form1::tcLang_KeyDown);
            this->tcLang->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &Form1::tcLang_KeyPress);
            this->tcLang->KeyUp += gcnew System::Windows::Forms::KeyEventHandler(this, &Form1::tcLang_KeyUp);
            // 
            // tabControl2
            // 
            this->tabControl2->Controls->Add(this->tabDialogsMenu);
            this->tabControl2->Controls->Add(this->tabInfo);
            this->tabControl2->Controls->Add(this->tabDialog);
            this->tabControl2->Controls->Add(this->tabAnswer);
            this->tabControl2->Controls->Add(this->tabDemand);
            this->tabControl2->Controls->Add(this->tabResult);
            this->tabControl2->Controls->Add(this->tabText);
            this->tabControl2->Controls->Add(this->tabComment);
            this->tabControl2->Dock = System::Windows::Forms::DockStyle::Bottom;
            this->tabControl2->Location = System::Drawing::Point(0, 216);
            this->tabControl2->Name = L"tabControl2";
            this->tabControl2->SelectedIndex = 0;
            this->tabControl2->Size = System::Drawing::Size(684, 225);
            this->tabControl2->TabIndex = 2;
            // 
            // tabDialogsMenu
            // 
            this->tabDialogsMenu->Controls->Add(this->btnTreeToMain);
            this->tabDialogsMenu->Controls->Add(this->btnDialogAddTree);
            this->tabDialogsMenu->Controls->Add(this->lDlgName);
            this->tabDialogsMenu->Controls->Add(this->label24);
            this->tabDialogsMenu->Controls->Add(this->btnDlgSave);
            this->tabDialogsMenu->Controls->Add(this->gbDialogsTexts);
            this->tabDialogsMenu->Controls->Add(this->btnDlgSaveAs);
            this->tabDialogsMenu->Controls->Add(this->btnDlgLoad);
            this->tabDialogsMenu->Controls->Add(this->gbDialogsDB);
            this->tabDialogsMenu->Controls->Add(this->lErrVarCount);
            this->tabDialogsMenu->Controls->Add(this->label30);
            this->tabDialogsMenu->Controls->Add(this->lErrCount);
            this->tabDialogsMenu->Controls->Add(this->label27);
            this->tabDialogsMenu->Controls->Add(this->btnDialogCheck);
            this->tabDialogsMenu->Controls->Add(this->btnDialogClear);
            this->tabDialogsMenu->Location = System::Drawing::Point(4, 22);
            this->tabDialogsMenu->Name = L"tabDialogsMenu";
            this->tabDialogsMenu->Size = System::Drawing::Size(676, 199);
            this->tabDialogsMenu->TabIndex = 4;
            this->tabDialogsMenu->Text = L"Опции";
            this->tabDialogsMenu->UseVisualStyleBackColor = true;
            this->tabDialogsMenu->Click += gcnew System::EventHandler(this, &Form1::tabDialogsMenu_Click);
            // 
            // btnTreeToMain
            // 
            this->btnTreeToMain->Location = System::Drawing::Point(230, 63);
            this->btnTreeToMain->Name = L"btnTreeToMain";
            this->btnTreeToMain->Size = System::Drawing::Size(107, 35);
            this->btnTreeToMain->TabIndex = 41;
            this->btnTreeToMain->Text = L"Сделать дерево основным";
            this->btnTreeToMain->UseVisualStyleBackColor = true;
            this->btnTreeToMain->Click += gcnew System::EventHandler(this, &Form1::btnTreeToMain_Click);
            // 
            // btnDialogAddTree
            // 
            this->btnDialogAddTree->Location = System::Drawing::Point(117, 63);
            this->btnDialogAddTree->Name = L"btnDialogAddTree";
            this->btnDialogAddTree->Size = System::Drawing::Size(107, 35);
            this->btnDialogAddTree->TabIndex = 40;
            this->btnDialogAddTree->Text = L"Добавить дерево";
            this->btnDialogAddTree->UseVisualStyleBackColor = true;
            this->btnDialogAddTree->Click += gcnew System::EventHandler(this, &Form1::btnDialogAddTree_Click);
            // 
            // lDlgName
            // 
            this->lDlgName->AutoSize = true;
            this->lDlgName->Location = System::Drawing::Point(98, 6);
            this->lDlgName->Name = L"lDlgName";
            this->lDlgName->Size = System::Drawing::Size(0, 13);
            this->lDlgName->TabIndex = 39;
            // 
            // label24
            // 
            this->label24->AutoSize = true;
            this->label24->Location = System::Drawing::Point(4, 6);
            this->label24->Name = L"label24";
            this->label24->Size = System::Drawing::Size(93, 13);
            this->label24->TabIndex = 38;
            this->label24->Text = L"Текущий диалог:";
            this->label24->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
            // 
            // btnDlgSave
            // 
            this->btnDlgSave->Location = System::Drawing::Point(117, 22);
            this->btnDlgSave->Name = L"btnDlgSave";
            this->btnDlgSave->Size = System::Drawing::Size(107, 35);
            this->btnDlgSave->TabIndex = 37;
            this->btnDlgSave->Text = L"Сохранить";
            this->btnDlgSave->UseVisualStyleBackColor = true;
            this->btnDlgSave->Click += gcnew System::EventHandler(this, &Form1::btnDlgSave_Click_1);
            // 
            // gbDialogsTexts
            // 
            this->gbDialogsTexts->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->gbDialogsTexts->Controls->Add(this->btnTextCheck);
            this->gbDialogsTexts->Controls->Add(this->btnReloadTexts);
            this->gbDialogsTexts->Controls->Add(this->txtDialogsLang);
            this->gbDialogsTexts->Controls->Add(this->label42);
            this->gbDialogsTexts->Location = System::Drawing::Point(182, 102);
            this->gbDialogsTexts->Name = L"gbDialogsTexts";
            this->gbDialogsTexts->Size = System::Drawing::Size(169, 89);
            this->gbDialogsTexts->TabIndex = 36;
            this->gbDialogsTexts->TabStop = false;
            this->gbDialogsTexts->Text = L"Тексты";
            this->gbDialogsTexts->Visible = false;
            // 
            // btnTextCheck
            // 
            this->btnTextCheck->Location = System::Drawing::Point(6, 61);
            this->btnTextCheck->Name = L"btnTextCheck";
            this->btnTextCheck->Size = System::Drawing::Size(157, 20);
            this->btnTextCheck->TabIndex = 35;
            this->btnTextCheck->Text = L"Удалить битые ссылки";
            this->btnTextCheck->UseVisualStyleBackColor = true;
            this->btnTextCheck->Click += gcnew System::EventHandler(this, &Form1::btnTextCheck_Click);
            // 
            // btnReloadTexts
            // 
            this->btnReloadTexts->Location = System::Drawing::Point(6, 38);
            this->btnReloadTexts->Name = L"btnReloadTexts";
            this->btnReloadTexts->Size = System::Drawing::Size(157, 20);
            this->btnReloadTexts->TabIndex = 34;
            this->btnReloadTexts->Text = L"Перегрузить тексты";
            this->btnReloadTexts->UseVisualStyleBackColor = true;
            this->btnReloadTexts->Click += gcnew System::EventHandler(this, &Form1::btnReloadTexts_Click);
            // 
            // txtDialogsLang
            // 
            this->txtDialogsLang->Location = System::Drawing::Point(47, 12);
            this->txtDialogsLang->MaxLength = 4;
            this->txtDialogsLang->Name = L"txtDialogsLang";
            this->txtDialogsLang->Size = System::Drawing::Size(50, 20);
            this->txtDialogsLang->TabIndex = 33;
            // 
            // label42
            // 
            this->label42->AutoSize = true;
            this->label42->Location = System::Drawing::Point(6, 14);
            this->label42->Name = L"label42";
            this->label42->Size = System::Drawing::Size(35, 13);
            this->label42->TabIndex = 32;
            this->label42->Text = L"Язык";
            // 
            // btnDlgSaveAs
            // 
            this->btnDlgSaveAs->Location = System::Drawing::Point(230, 22);
            this->btnDlgSaveAs->Name = L"btnDlgSaveAs";
            this->btnDlgSaveAs->Size = System::Drawing::Size(107, 35);
            this->btnDlgSaveAs->TabIndex = 35;
            this->btnDlgSaveAs->Text = L"Сохранить как";
            this->btnDlgSaveAs->UseVisualStyleBackColor = true;
            this->btnDlgSaveAs->Click += gcnew System::EventHandler(this, &Form1::btnDlgSave_Click);
            // 
            // btnDlgLoad
            // 
            this->btnDlgLoad->Location = System::Drawing::Point(4, 22);
            this->btnDlgLoad->Name = L"btnDlgLoad";
            this->btnDlgLoad->Size = System::Drawing::Size(107, 35);
            this->btnDlgLoad->TabIndex = 34;
            this->btnDlgLoad->Text = L"Загрузить";
            this->btnDlgLoad->UseVisualStyleBackColor = true;
            this->btnDlgLoad->Click += gcnew System::EventHandler(this, &Form1::btnDlgLoad_Click);
            // 
            // gbDialogsDB
            // 
            this->gbDialogsDB->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->gbDialogsDB->Controls->Add(this->btnDialogErase);
            this->gbDialogsDB->Controls->Add(this->numDialogsId);
            this->gbDialogsDB->Controls->Add(this->txtDialogName);
            this->gbDialogsDB->Controls->Add(this->label36);
            this->gbDialogsDB->Controls->Add(this->label35);
            this->gbDialogsDB->Controls->Add(this->lbDialogsList);
            this->gbDialogsDB->Controls->Add(this->btnDialogSave);
            this->gbDialogsDB->Location = System::Drawing::Point(357, 102);
            this->gbDialogsDB->Name = L"gbDialogsDB";
            this->gbDialogsDB->Size = System::Drawing::Size(269, 138);
            this->gbDialogsDB->TabIndex = 33;
            this->gbDialogsDB->TabStop = false;
            this->gbDialogsDB->Text = L"База данных";
            this->gbDialogsDB->Visible = false;
            // 
            // btnDialogErase
            // 
            this->btnDialogErase->Location = System::Drawing::Point(7, 109);
            this->btnDialogErase->Name = L"btnDialogErase";
            this->btnDialogErase->Size = System::Drawing::Size(99, 25);
            this->btnDialogErase->TabIndex = 39;
            this->btnDialogErase->Text = L"Удалить";
            this->btnDialogErase->UseVisualStyleBackColor = true;
            this->btnDialogErase->Click += gcnew System::EventHandler(this, &Form1::btnDialogErase_Click);
            // 
            // numDialogsId
            // 
            this->numDialogsId->Location = System::Drawing::Point(7, 59);
            this->numDialogsId->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {10000000, 0, 0, 0});
            this->numDialogsId->Name = L"numDialogsId";
            this->numDialogsId->Size = System::Drawing::Size(99, 20);
            this->numDialogsId->TabIndex = 38;
            // 
            // txtDialogName
            // 
            this->txtDialogName->Location = System::Drawing::Point(7, 27);
            this->txtDialogName->Name = L"txtDialogName";
            this->txtDialogName->Size = System::Drawing::Size(99, 20);
            this->txtDialogName->TabIndex = 37;
            // 
            // label36
            // 
            this->label36->AutoSize = true;
            this->label36->Location = System::Drawing::Point(4, 45);
            this->label36->Name = L"label36";
            this->label36->Size = System::Drawing::Size(41, 13);
            this->label36->TabIndex = 36;
            this->label36->Text = L"Номер";
            // 
            // label35
            // 
            this->label35->AutoSize = true;
            this->label35->Location = System::Drawing::Point(4, 11);
            this->label35->Name = L"label35";
            this->label35->Size = System::Drawing::Size(64, 13);
            this->label35->TabIndex = 35;
            this->label35->Text = L"Имя файла";
            // 
            // lbDialogsList
            // 
            this->lbDialogsList->FormattingEnabled = true;
            this->lbDialogsList->Location = System::Drawing::Point(112, 12);
            this->lbDialogsList->Name = L"lbDialogsList";
            this->lbDialogsList->Size = System::Drawing::Size(150, 121);
            this->lbDialogsList->TabIndex = 34;
            this->lbDialogsList->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::lbDialogsList_SelectedIndexChanged);
            this->lbDialogsList->MouseDoubleClick += gcnew System::Windows::Forms::MouseEventHandler(this, &Form1::lbDialogsList_MouseDoubleClick);
            // 
            // btnDialogSave
            // 
            this->btnDialogSave->Location = System::Drawing::Point(7, 83);
            this->btnDialogSave->Name = L"btnDialogSave";
            this->btnDialogSave->Size = System::Drawing::Size(99, 25);
            this->btnDialogSave->TabIndex = 33;
            this->btnDialogSave->Text = L"Сохранить";
            this->btnDialogSave->UseVisualStyleBackColor = true;
            this->btnDialogSave->Click += gcnew System::EventHandler(this, &Form1::btnDialogSave_Click);
            // 
            // lErrVarCount
            // 
            this->lErrVarCount->AutoSize = true;
            this->lErrVarCount->Location = System::Drawing::Point(163, 102);
            this->lErrVarCount->Name = L"lErrVarCount";
            this->lErrVarCount->Size = System::Drawing::Size(13, 13);
            this->lErrVarCount->TabIndex = 23;
            this->lErrVarCount->Text = L"0";
            this->lErrVarCount->Visible = false;
            // 
            // label30
            // 
            this->label30->AutoSize = true;
            this->label30->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(192)), static_cast<System::Int32>(static_cast<System::Byte>(0)), 
                static_cast<System::Int32>(static_cast<System::Byte>(0)));
            this->label30->Location = System::Drawing::Point(11, 102);
            this->label30->Name = L"label30";
            this->label30->Size = System::Drawing::Size(146, 13);
            this->label30->TabIndex = 22;
            this->label30->Text = L"Неизвестные переменные:";
            this->label30->Visible = false;
            // 
            // lErrCount
            // 
            this->lErrCount->AutoSize = true;
            this->lErrCount->Location = System::Drawing::Point(163, 120);
            this->lErrCount->Name = L"lErrCount";
            this->lErrCount->Size = System::Drawing::Size(13, 13);
            this->lErrCount->TabIndex = 21;
            this->lErrCount->Text = L"0";
            this->lErrCount->Visible = false;
            // 
            // label27
            // 
            this->label27->AutoSize = true;
            this->label27->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(192)), static_cast<System::Int32>(static_cast<System::Byte>(0)), 
                static_cast<System::Int32>(static_cast<System::Byte>(0)));
            this->label27->Location = System::Drawing::Point(11, 120);
            this->label27->Name = L"label27";
            this->label27->Size = System::Drawing::Size(128, 13);
            this->label27->TabIndex = 20;
            this->label27->Text = L"Число кривых ответов: ";
            this->label27->Visible = false;
            // 
            // btnDialogCheck
            // 
            this->btnDialogCheck->Location = System::Drawing::Point(60, 132);
            this->btnDialogCheck->Name = L"btnDialogCheck";
            this->btnDialogCheck->Size = System::Drawing::Size(107, 37);
            this->btnDialogCheck->TabIndex = 15;
            this->btnDialogCheck->Text = L"Проверить диалог";
            this->btnDialogCheck->UseVisualStyleBackColor = true;
            this->btnDialogCheck->Visible = false;
            this->btnDialogCheck->Click += gcnew System::EventHandler(this, &Form1::btnDialogCheck_Click);
            // 
            // btnDialogClear
            // 
            this->btnDialogClear->Location = System::Drawing::Point(4, 63);
            this->btnDialogClear->Name = L"btnDialogClear";
            this->btnDialogClear->Size = System::Drawing::Size(107, 35);
            this->btnDialogClear->TabIndex = 17;
            this->btnDialogClear->Text = L"Очистить";
            this->btnDialogClear->UseVisualStyleBackColor = true;
            this->btnDialogClear->Click += gcnew System::EventHandler(this, &Form1::btnDialogClear_Click);
            // 
            // tabInfo
            // 
            this->tabInfo->Controls->Add(this->tInfoAvatar);
            this->tabInfo->Controls->Add(this->lInfoAvatar);
            this->tabInfo->Controls->Add(this->tInfoName);
            this->tabInfo->Controls->Add(this->groupBox1);
            this->tabInfo->Controls->Add(this->lInfoName);
            this->tabInfo->Location = System::Drawing::Point(4, 22);
            this->tabInfo->Name = L"tabInfo";
            this->tabInfo->Size = System::Drawing::Size(676, 199);
            this->tabInfo->TabIndex = 5;
            this->tabInfo->Text = L"Инфо";
            this->tabInfo->UseVisualStyleBackColor = true;
            // 
            // tInfoAvatar
            // 
            this->tInfoAvatar->Anchor = System::Windows::Forms::AnchorStyles::Right;
            this->tInfoAvatar->Location = System::Drawing::Point(425, 20);
            this->tInfoAvatar->Name = L"tInfoAvatar";
            this->tInfoAvatar->Size = System::Drawing::Size(247, 20);
            this->tInfoAvatar->TabIndex = 13;
            this->tInfoAvatar->TextChanged += gcnew System::EventHandler(this, &Form1::tInfoName_TextChanged);
            // 
            // lInfoAvatar
            // 
            this->lInfoAvatar->Anchor = System::Windows::Forms::AnchorStyles::Right;
            this->lInfoAvatar->AutoSize = true;
            this->lInfoAvatar->Location = System::Drawing::Point(446, 4);
            this->lInfoAvatar->Name = L"lInfoAvatar";
            this->lInfoAvatar->Size = System::Drawing::Size(43, 13);
            this->lInfoAvatar->TabIndex = 12;
            this->lInfoAvatar->Text = L"Аватар";
            // 
            // tInfoName
            // 
            this->tInfoName->Anchor = System::Windows::Forms::AnchorStyles::Right;
            this->tInfoName->Location = System::Drawing::Point(6, 20);
            this->tInfoName->Name = L"tInfoName";
            this->tInfoName->Size = System::Drawing::Size(413, 20);
            this->tInfoName->TabIndex = 11;
            this->tInfoName->TextChanged += gcnew System::EventHandler(this, &Form1::tInfoName_TextChanged);
            // 
            // groupBox1
            // 
            this->groupBox1->Controls->Add(this->lInfoNum);
            this->groupBox1->Controls->Add(this->lInfoAbout);
            this->groupBox1->Controls->Add(this->lInfoFull);
            this->groupBox1->Controls->Add(this->lInfoShort);
            this->groupBox1->Controls->Add(this->numInfoNum);
            this->groupBox1->Controls->Add(this->rtInfoFull);
            this->groupBox1->Controls->Add(this->rtInfoShort);
            this->groupBox1->Controls->Add(this->rbInfoCriticalDead);
            this->groupBox1->Controls->Add(this->rbInfoDead);
            this->groupBox1->Controls->Add(this->rbInfoKnockout);
            this->groupBox1->Controls->Add(this->rbInfoLive);
            this->groupBox1->Dock = System::Windows::Forms::DockStyle::Bottom;
            this->groupBox1->Location = System::Drawing::Point(0, 47);
            this->groupBox1->Name = L"groupBox1";
            this->groupBox1->Size = System::Drawing::Size(676, 152);
            this->groupBox1->TabIndex = 10;
            this->groupBox1->TabStop = false;
            this->groupBox1->Text = L"Описания";
            // 
            // lInfoNum
            // 
            this->lInfoNum->AutoSize = true;
            this->lInfoNum->Location = System::Drawing::Point(17, 89);
            this->lInfoNum->Name = L"lInfoNum";
            this->lInfoNum->Size = System::Drawing::Size(92, 13);
            this->lInfoNum->TabIndex = 11;
            this->lInfoNum->Text = L"Номер описания";
            // 
            // lInfoAbout
            // 
            this->lInfoAbout->AutoSize = true;
            this->lInfoAbout->Location = System::Drawing::Point(135, 83);
            this->lInfoAbout->Name = L"lInfoAbout";
            this->lInfoAbout->Size = System::Drawing::Size(25, 13);
            this->lInfoAbout->TabIndex = 10;
            this->lInfoAbout->Text = L"Info";
            // 
            // lInfoFull
            // 
            this->lInfoFull->AutoSize = true;
            this->lInfoFull->Location = System::Drawing::Point(341, 10);
            this->lInfoFull->Name = L"lInfoFull";
            this->lInfoFull->Size = System::Drawing::Size(45, 13);
            this->lInfoFull->TabIndex = 9;
            this->lInfoFull->Text = L"Полное";
            // 
            // lInfoShort
            // 
            this->lInfoShort->AutoSize = true;
            this->lInfoShort->Location = System::Drawing::Point(140, 10);
            this->lInfoShort->Name = L"lInfoShort";
            this->lInfoShort->Size = System::Drawing::Size(49, 13);
            this->lInfoShort->TabIndex = 8;
            this->lInfoShort->Text = L"Краткое";
            // 
            // numInfoNum
            // 
            this->numInfoNum->Location = System::Drawing::Point(39, 105);
            this->numInfoNum->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) {1, 0, 0, 0});
            this->numInfoNum->Name = L"numInfoNum";
            this->numInfoNum->Size = System::Drawing::Size(38, 20);
            this->numInfoNum->TabIndex = 7;
            this->numInfoNum->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {1, 0, 0, 0});
            this->numInfoNum->ValueChanged += gcnew System::EventHandler(this, &Form1::numInfoNum_ValueChanged);
            // 
            // rtInfoFull
            // 
            this->rtInfoFull->Location = System::Drawing::Point(311, 26);
            this->rtInfoFull->Name = L"rtInfoFull";
            this->rtInfoFull->Size = System::Drawing::Size(348, 55);
            this->rtInfoFull->TabIndex = 5;
            this->rtInfoFull->Text = L"";
            this->rtInfoFull->TextChanged += gcnew System::EventHandler(this, &Form1::tInfoName_TextChanged);
            // 
            // rtInfoShort
            // 
            this->rtInfoShort->Location = System::Drawing::Point(135, 26);
            this->rtInfoShort->Name = L"rtInfoShort";
            this->rtInfoShort->Size = System::Drawing::Size(170, 54);
            this->rtInfoShort->TabIndex = 4;
            this->rtInfoShort->Text = L"";
            this->rtInfoShort->TextChanged += gcnew System::EventHandler(this, &Form1::tInfoName_TextChanged);
            // 
            // rbInfoCriticalDead
            // 
            this->rbInfoCriticalDead->AutoSize = true;
            this->rbInfoCriticalDead->Location = System::Drawing::Point(6, 69);
            this->rbInfoCriticalDead->Name = L"rbInfoCriticalDead";
            this->rbInfoCriticalDead->Size = System::Drawing::Size(117, 17);
            this->rbInfoCriticalDead->TabIndex = 3;
            this->rbInfoCriticalDead->Text = L"Жестокая смерть";
            this->rbInfoCriticalDead->UseVisualStyleBackColor = true;
            this->rbInfoCriticalDead->CheckedChanged += gcnew System::EventHandler(this, &Form1::rbInfoLive_CheckedChanged);
            // 
            // rbInfoDead
            // 
            this->rbInfoDead->AutoSize = true;
            this->rbInfoDead->Location = System::Drawing::Point(6, 51);
            this->rbInfoDead->Name = L"rbInfoDead";
            this->rbInfoDead->Size = System::Drawing::Size(71, 17);
            this->rbInfoDead->TabIndex = 2;
            this->rbInfoDead->Text = L"Мертвый";
            this->rbInfoDead->UseVisualStyleBackColor = true;
            this->rbInfoDead->CheckedChanged += gcnew System::EventHandler(this, &Form1::rbInfoLive_CheckedChanged);
            // 
            // rbInfoKnockout
            // 
            this->rbInfoKnockout->AutoSize = true;
            this->rbInfoKnockout->Location = System::Drawing::Point(6, 33);
            this->rbInfoKnockout->Name = L"rbInfoKnockout";
            this->rbInfoKnockout->Size = System::Drawing::Size(75, 17);
            this->rbInfoKnockout->TabIndex = 1;
            this->rbInfoKnockout->Text = L"В накауте";
            this->rbInfoKnockout->UseVisualStyleBackColor = true;
            this->rbInfoKnockout->CheckedChanged += gcnew System::EventHandler(this, &Form1::rbInfoLive_CheckedChanged);
            // 
            // rbInfoLive
            // 
            this->rbInfoLive->AutoSize = true;
            this->rbInfoLive->Checked = true;
            this->rbInfoLive->Location = System::Drawing::Point(6, 16);
            this->rbInfoLive->Name = L"rbInfoLive";
            this->rbInfoLive->Size = System::Drawing::Size(60, 17);
            this->rbInfoLive->TabIndex = 0;
            this->rbInfoLive->TabStop = true;
            this->rbInfoLive->Text = L"Живой";
            this->rbInfoLive->UseVisualStyleBackColor = true;
            this->rbInfoLive->CheckedChanged += gcnew System::EventHandler(this, &Form1::rbInfoLive_CheckedChanged);
            // 
            // lInfoName
            // 
            this->lInfoName->Anchor = System::Windows::Forms::AnchorStyles::Right;
            this->lInfoName->AutoSize = true;
            this->lInfoName->Location = System::Drawing::Point(27, 4);
            this->lInfoName->Name = L"lInfoName";
            this->lInfoName->Size = System::Drawing::Size(29, 13);
            this->lInfoName->TabIndex = 9;
            this->lInfoName->Text = L"Имя";
            // 
            // tabDialog
            // 
            this->tabDialog->Controls->Add(this->cbDialogNoShuffle);
            this->tabDialog->Controls->Add(this->label40);
            this->tabDialog->Controls->Add(this->label26);
            this->tabDialog->Controls->Add(this->numIDdialog);
            this->tabDialog->Controls->Add(this->cbAID);
            this->tabDialog->Controls->Add(this->label1);
            this->tabDialog->Controls->Add(this->btnAddDialog);
            this->tabDialog->Controls->Add(this->txtDialog);
            this->tabDialog->Controls->Add(this->cbAction_no_answ);
            this->tabDialog->Location = System::Drawing::Point(4, 22);
            this->tabDialog->Name = L"tabDialog";
            this->tabDialog->Padding = System::Windows::Forms::Padding(3);
            this->tabDialog->Size = System::Drawing::Size(676, 199);
            this->tabDialog->TabIndex = 0;
            this->tabDialog->Text = L"Диалог";
            this->tabDialog->UseVisualStyleBackColor = true;
            // 
            // cbDialogNoShuffle
            // 
            this->cbDialogNoShuffle->AutoSize = true;
            this->cbDialogNoShuffle->Location = System::Drawing::Point(332, 129);
            this->cbDialogNoShuffle->Name = L"cbDialogNoShuffle";
            this->cbDialogNoShuffle->Size = System::Drawing::Size(157, 17);
            this->cbDialogNoShuffle->TabIndex = 51;
            this->cbDialogNoShuffle->Text = L"Не перемешивать ответы";
            this->cbDialogNoShuffle->UseVisualStyleBackColor = true;
            // 
            // label40
            // 
            this->label40->AutoSize = true;
            this->label40->Location = System::Drawing::Point(6, 3);
            this->label40->Name = L"label40";
            this->label40->Size = System::Drawing::Size(81, 13);
            this->label40->TabIndex = 50;
            this->label40->Text = L"Текст диалога";
            // 
            // label26
            // 
            this->label26->AutoSize = true;
            this->label26->Location = System::Drawing::Point(330, 86);
            this->label26->Name = L"label26";
            this->label26->Size = System::Drawing::Size(34, 13);
            this->label26->TabIndex = 48;
            this->label26->Text = L"Script";
            // 
            // numIDdialog
            // 
            this->numIDdialog->Location = System::Drawing::Point(332, 22);
            this->numIDdialog->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {999, 0, 0, 0});
            this->numIDdialog->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) {2, 0, 0, 0});
            this->numIDdialog->Name = L"numIDdialog";
            this->numIDdialog->Size = System::Drawing::Size(82, 20);
            this->numIDdialog->TabIndex = 45;
            this->numIDdialog->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {2, 0, 0, 0});
            // 
            // cbAID
            // 
            this->cbAID->AutoSize = true;
            this->cbAID->Checked = true;
            this->cbAID->CheckState = System::Windows::Forms::CheckState::Checked;
            this->cbAID->Location = System::Drawing::Point(420, 22);
            this->cbAID->Name = L"cbAID";
            this->cbAID->Size = System::Drawing::Size(62, 17);
            this->cbAID->TabIndex = 44;
            this->cbAID->Text = L"Авто Id";
            this->cbAID->UseVisualStyleBackColor = true;
            // 
            // label1
            // 
            this->label1->AutoSize = true;
            this->label1->Location = System::Drawing::Point(329, 6);
            this->label1->Name = L"label1";
            this->label1->Size = System::Drawing::Size(85, 13);
            this->label1->TabIndex = 43;
            this->label1->Text = L"Номер диалога";
            // 
            // btnAddDialog
            // 
            this->btnAddDialog->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->btnAddDialog->Location = System::Drawing::Point(531, 6);
            this->btnAddDialog->Name = L"btnAddDialog";
            this->btnAddDialog->Size = System::Drawing::Size(139, 54);
            this->btnAddDialog->TabIndex = 42;
            this->btnAddDialog->Text = L"Добавить диалог";
            this->btnAddDialog->UseVisualStyleBackColor = true;
            this->btnAddDialog->Click += gcnew System::EventHandler(this, &Form1::btnAddDialog_Click);
            // 
            // txtDialog
            // 
            this->txtDialog->Location = System::Drawing::Point(3, 19);
            this->txtDialog->MaxLength = 10000;
            this->txtDialog->Name = L"txtDialog";
            this->txtDialog->Size = System::Drawing::Size(324, 163);
            this->txtDialog->TabIndex = 41;
            this->txtDialog->Text = L"";
            // 
            // cbAction_no_answ
            // 
            this->cbAction_no_answ->FormattingEnabled = true;
            this->cbAction_no_answ->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"None", L"Attack"});
            this->cbAction_no_answ->Location = System::Drawing::Point(332, 102);
            this->cbAction_no_answ->Name = L"cbAction_no_answ";
            this->cbAction_no_answ->Size = System::Drawing::Size(333, 21);
            this->cbAction_no_answ->TabIndex = 49;
            // 
            // tabAnswer
            // 
            this->tabAnswer->Controls->Add(this->cbLinkOther);
            this->tabAnswer->Controls->Add(this->rbLinkOther);
            this->tabAnswer->Controls->Add(this->rbLinkId);
            this->tabAnswer->Controls->Add(this->label39);
            this->tabAnswer->Controls->Add(this->txtAnswer);
            this->tabAnswer->Controls->Add(this->numLink);
            this->tabAnswer->Controls->Add(this->btnAddAnswer);
            this->tabAnswer->Location = System::Drawing::Point(4, 22);
            this->tabAnswer->Name = L"tabAnswer";
            this->tabAnswer->Padding = System::Windows::Forms::Padding(3);
            this->tabAnswer->Size = System::Drawing::Size(676, 199);
            this->tabAnswer->TabIndex = 1;
            this->tabAnswer->Text = L"Ответ";
            this->tabAnswer->UseVisualStyleBackColor = true;
            // 
            // cbLinkOther
            // 
            this->cbLinkOther->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbLinkOther->FormattingEnabled = true;
            this->cbLinkOther->Items->AddRange(gcnew cli::array< System::Object^  >(4) {L"Закрыть диалог", L"На предыдущий диалог", L"Бартер", 
                L"Атаковать"});
            this->cbLinkOther->Location = System::Drawing::Point(338, 91);
            this->cbLinkOther->Name = L"cbLinkOther";
            this->cbLinkOther->Size = System::Drawing::Size(169, 21);
            this->cbLinkOther->TabIndex = 44;
            // 
            // rbLinkOther
            // 
            this->rbLinkOther->AutoSize = true;
            this->rbLinkOther->Location = System::Drawing::Point(338, 68);
            this->rbLinkOther->Name = L"rbLinkOther";
            this->rbLinkOther->Size = System::Drawing::Size(62, 17);
            this->rbLinkOther->TabIndex = 43;
            this->rbLinkOther->Text = L"Другой";
            this->rbLinkOther->UseVisualStyleBackColor = true;
            // 
            // rbLinkId
            // 
            this->rbLinkId->AutoSize = true;
            this->rbLinkId->Checked = true;
            this->rbLinkId->Location = System::Drawing::Point(338, 19);
            this->rbLinkId->Name = L"rbLinkId";
            this->rbLinkId->Size = System::Drawing::Size(75, 17);
            this->rbLinkId->TabIndex = 42;
            this->rbLinkId->TabStop = true;
            this->rbLinkId->Text = L"К диалогу";
            this->rbLinkId->UseVisualStyleBackColor = true;
            // 
            // label39
            // 
            this->label39->AutoSize = true;
            this->label39->Location = System::Drawing::Point(4, 3);
            this->label39->Name = L"label39";
            this->label39->Size = System::Drawing::Size(74, 13);
            this->label39->TabIndex = 41;
            this->label39->Text = L"Текст ответа";
            // 
            // txtAnswer
            // 
            this->txtAnswer->Location = System::Drawing::Point(3, 19);
            this->txtAnswer->Name = L"txtAnswer";
            this->txtAnswer->Size = System::Drawing::Size(326, 116);
            this->txtAnswer->TabIndex = 40;
            this->txtAnswer->Text = L"";
            // 
            // numLink
            // 
            this->numLink->Location = System::Drawing::Point(338, 42);
            this->numLink->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {65000, 0, 0, 0});
            this->numLink->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) {2, 0, 0, 0});
            this->numLink->Name = L"numLink";
            this->numLink->Size = System::Drawing::Size(99, 20);
            this->numLink->TabIndex = 39;
            this->numLink->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {2, 0, 0, 0});
            // 
            // btnAddAnswer
            // 
            this->btnAddAnswer->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
            this->btnAddAnswer->Enabled = false;
            this->btnAddAnswer->Location = System::Drawing::Point(528, 6);
            this->btnAddAnswer->Name = L"btnAddAnswer";
            this->btnAddAnswer->Size = System::Drawing::Size(142, 55);
            this->btnAddAnswer->TabIndex = 37;
            this->btnAddAnswer->Text = L"Добавить ответ";
            this->btnAddAnswer->UseVisualStyleBackColor = true;
            this->btnAddAnswer->Click += gcnew System::EventHandler(this, &Form1::btnAddAnswer_Click);
            // 
            // tabDemand
            // 
            this->tabDemand->Controls->Add(this->cbDemandValue);
            this->tabDemand->Controls->Add(this->btnDemandOr);
            this->tabDemand->Controls->Add(this->cbDemandNoRecheck);
            this->tabDemand->Controls->Add(this->gbDemandScript);
            this->tabDemand->Controls->Add(this->cbDemandItem);
            this->tabDemand->Controls->Add(this->txtDemandScript);
            this->tabDemand->Controls->Add(this->rbDemandScript);
            this->tabDemand->Controls->Add(this->panel2);
            this->tabDemand->Controls->Add(this->btnDemandAdd);
            this->tabDemand->Controls->Add(this->label19);
            this->tabDemand->Controls->Add(this->cbDemandVars);
            this->tabDemand->Controls->Add(this->cbDemandParam);
            this->tabDemand->Controls->Add(this->rbDemandVar);
            this->tabDemand->Controls->Add(this->rbDemandParam);
            this->tabDemand->Controls->Add(this->btnDemandOper);
            this->tabDemand->Controls->Add(this->rbDemandItem);
            this->tabDemand->Location = System::Drawing::Point(4, 22);
            this->tabDemand->Name = L"tabDemand";
            this->tabDemand->Size = System::Drawing::Size(676, 199);
            this->tabDemand->TabIndex = 2;
            this->tabDemand->Text = L"Условие";
            this->tabDemand->UseVisualStyleBackColor = true;
            // 
            // cbDemandValue
            // 
            this->cbDemandValue->DropDownHeight = 400;
            this->cbDemandValue->FormattingEnabled = true;
            this->cbDemandValue->IntegralHeight = false;
            this->cbDemandValue->Location = System::Drawing::Point(109, 122);
            this->cbDemandValue->Name = L"cbDemandValue";
            this->cbDemandValue->Size = System::Drawing::Size(198, 21);
            this->cbDemandValue->TabIndex = 120;
            // 
            // btnDemandOr
            // 
            this->btnDemandOr->Location = System::Drawing::Point(634, 3);
            this->btnDemandOr->Name = L"btnDemandOr";
            this->btnDemandOr->Size = System::Drawing::Size(38, 26);
            this->btnDemandOr->TabIndex = 111;
            this->btnDemandOr->Text = L"Или";
            this->btnDemandOr->UseVisualStyleBackColor = true;
            this->btnDemandOr->Click += gcnew System::EventHandler(this, &Form1::btnDemandAdd_Click);
            // 
            // cbDemandNoRecheck
            // 
            this->cbDemandNoRecheck->AutoSize = true;
            this->cbDemandNoRecheck->Location = System::Drawing::Point(13, 149);
            this->cbDemandNoRecheck->Name = L"cbDemandNoRecheck";
            this->cbDemandNoRecheck->Size = System::Drawing::Size(82, 17);
            this->cbDemandNoRecheck->TabIndex = 110;
            this->cbDemandNoRecheck->Text = L"No recheck";
            this->cbDemandNoRecheck->UseVisualStyleBackColor = true;
            // 
            // gbDemandScript
            // 
            this->gbDemandScript->Controls->Add(this->cbDemandSValue4);
            this->gbDemandScript->Controls->Add(this->cbDemandSValue3);
            this->gbDemandScript->Controls->Add(this->cbDemandSValue2);
            this->gbDemandScript->Controls->Add(this->cbDemandSValue1);
            this->gbDemandScript->Controls->Add(this->cbDemandSValue0);
            this->gbDemandScript->Controls->Add(this->rbDemandScriptVal0);
            this->gbDemandScript->Controls->Add(this->rbDemandScriptVal5);
            this->gbDemandScript->Controls->Add(this->rbDemandScriptVal4);
            this->gbDemandScript->Controls->Add(this->rbDemandScriptVal3);
            this->gbDemandScript->Controls->Add(this->rbDemandScriptVal2);
            this->gbDemandScript->Controls->Add(this->rbDemandScriptVal1);
            this->gbDemandScript->Location = System::Drawing::Point(316, 33);
            this->gbDemandScript->Name = L"gbDemandScript";
            this->gbDemandScript->Size = System::Drawing::Size(356, 128);
            this->gbDemandScript->TabIndex = 107;
            this->gbDemandScript->TabStop = false;
            this->gbDemandScript->Text = L"Аргументы скрипта";
            this->gbDemandScript->Visible = false;
            // 
            // cbDemandSValue4
            // 
            this->cbDemandSValue4->DropDownHeight = 400;
            this->cbDemandSValue4->FormattingEnabled = true;
            this->cbDemandSValue4->IntegralHeight = false;
            this->cbDemandSValue4->Location = System::Drawing::Point(6, 110);
            this->cbDemandSValue4->Name = L"cbDemandSValue4";
            this->cbDemandSValue4->Size = System::Drawing::Size(306, 21);
            this->cbDemandSValue4->TabIndex = 123;
            // 
            // cbDemandSValue3
            // 
            this->cbDemandSValue3->DropDownHeight = 400;
            this->cbDemandSValue3->FormattingEnabled = true;
            this->cbDemandSValue3->IntegralHeight = false;
            this->cbDemandSValue3->Location = System::Drawing::Point(6, 90);
            this->cbDemandSValue3->Name = L"cbDemandSValue3";
            this->cbDemandSValue3->Size = System::Drawing::Size(306, 21);
            this->cbDemandSValue3->TabIndex = 122;
            // 
            // cbDemandSValue2
            // 
            this->cbDemandSValue2->DropDownHeight = 400;
            this->cbDemandSValue2->FormattingEnabled = true;
            this->cbDemandSValue2->IntegralHeight = false;
            this->cbDemandSValue2->Location = System::Drawing::Point(6, 70);
            this->cbDemandSValue2->Name = L"cbDemandSValue2";
            this->cbDemandSValue2->Size = System::Drawing::Size(306, 21);
            this->cbDemandSValue2->TabIndex = 121;
            // 
            // cbDemandSValue1
            // 
            this->cbDemandSValue1->DropDownHeight = 400;
            this->cbDemandSValue1->FormattingEnabled = true;
            this->cbDemandSValue1->IntegralHeight = false;
            this->cbDemandSValue1->Location = System::Drawing::Point(6, 50);
            this->cbDemandSValue1->Name = L"cbDemandSValue1";
            this->cbDemandSValue1->Size = System::Drawing::Size(306, 21);
            this->cbDemandSValue1->TabIndex = 120;
            // 
            // cbDemandSValue0
            // 
            this->cbDemandSValue0->DropDownHeight = 400;
            this->cbDemandSValue0->FormattingEnabled = true;
            this->cbDemandSValue0->IntegralHeight = false;
            this->cbDemandSValue0->Location = System::Drawing::Point(6, 30);
            this->cbDemandSValue0->Name = L"cbDemandSValue0";
            this->cbDemandSValue0->Size = System::Drawing::Size(306, 21);
            this->cbDemandSValue0->TabIndex = 119;
            // 
            // rbDemandScriptVal0
            // 
            this->rbDemandScriptVal0->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->rbDemandScriptVal0->AutoSize = true;
            this->rbDemandScriptVal0->Checked = true;
            this->rbDemandScriptVal0->Location = System::Drawing::Point(238, 13);
            this->rbDemandScriptVal0->Name = L"rbDemandScriptVal0";
            this->rbDemandScriptVal0->Size = System::Drawing::Size(44, 17);
            this->rbDemandScriptVal0->TabIndex = 118;
            this->rbDemandScriptVal0->TabStop = true;
            this->rbDemandScriptVal0->Text = L"Нет";
            this->rbDemandScriptVal0->UseVisualStyleBackColor = true;
            // 
            // rbDemandScriptVal5
            // 
            this->rbDemandScriptVal5->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->rbDemandScriptVal5->AutoSize = true;
            this->rbDemandScriptVal5->Location = System::Drawing::Point(318, 112);
            this->rbDemandScriptVal5->Name = L"rbDemandScriptVal5";
            this->rbDemandScriptVal5->Size = System::Drawing::Size(31, 17);
            this->rbDemandScriptVal5->TabIndex = 117;
            this->rbDemandScriptVal5->Text = L"5";
            this->rbDemandScriptVal5->UseVisualStyleBackColor = true;
            // 
            // rbDemandScriptVal4
            // 
            this->rbDemandScriptVal4->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->rbDemandScriptVal4->AutoSize = true;
            this->rbDemandScriptVal4->Location = System::Drawing::Point(318, 93);
            this->rbDemandScriptVal4->Name = L"rbDemandScriptVal4";
            this->rbDemandScriptVal4->Size = System::Drawing::Size(31, 17);
            this->rbDemandScriptVal4->TabIndex = 116;
            this->rbDemandScriptVal4->Text = L"4";
            this->rbDemandScriptVal4->UseVisualStyleBackColor = true;
            // 
            // rbDemandScriptVal3
            // 
            this->rbDemandScriptVal3->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->rbDemandScriptVal3->AutoSize = true;
            this->rbDemandScriptVal3->Location = System::Drawing::Point(318, 73);
            this->rbDemandScriptVal3->Name = L"rbDemandScriptVal3";
            this->rbDemandScriptVal3->Size = System::Drawing::Size(31, 17);
            this->rbDemandScriptVal3->TabIndex = 115;
            this->rbDemandScriptVal3->Text = L"3";
            this->rbDemandScriptVal3->UseVisualStyleBackColor = true;
            // 
            // rbDemandScriptVal2
            // 
            this->rbDemandScriptVal2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->rbDemandScriptVal2->AutoSize = true;
            this->rbDemandScriptVal2->Location = System::Drawing::Point(318, 52);
            this->rbDemandScriptVal2->Name = L"rbDemandScriptVal2";
            this->rbDemandScriptVal2->Size = System::Drawing::Size(31, 17);
            this->rbDemandScriptVal2->TabIndex = 114;
            this->rbDemandScriptVal2->Text = L"2";
            this->rbDemandScriptVal2->UseVisualStyleBackColor = true;
            // 
            // rbDemandScriptVal1
            // 
            this->rbDemandScriptVal1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->rbDemandScriptVal1->AutoSize = true;
            this->rbDemandScriptVal1->Location = System::Drawing::Point(318, 31);
            this->rbDemandScriptVal1->Name = L"rbDemandScriptVal1";
            this->rbDemandScriptVal1->Size = System::Drawing::Size(31, 17);
            this->rbDemandScriptVal1->TabIndex = 113;
            this->rbDemandScriptVal1->Text = L"1";
            this->rbDemandScriptVal1->UseVisualStyleBackColor = true;
            // 
            // cbDemandItem
            // 
            this->cbDemandItem->DropDownHeight = 400;
            this->cbDemandItem->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbDemandItem->FormattingEnabled = true;
            this->cbDemandItem->IntegralHeight = false;
            this->cbDemandItem->Location = System::Drawing::Point(76, 56);
            this->cbDemandItem->Name = L"cbDemandItem";
            this->cbDemandItem->Size = System::Drawing::Size(231, 21);
            this->cbDemandItem->Sorted = true;
            this->cbDemandItem->TabIndex = 96;
            // 
            // txtDemandScript
            // 
            this->txtDemandScript->Location = System::Drawing::Point(339, 163);
            this->txtDemandScript->Name = L"txtDemandScript";
            this->txtDemandScript->Size = System::Drawing::Size(325, 20);
            this->txtDemandScript->TabIndex = 95;
            // 
            // rbDemandScript
            // 
            this->rbDemandScript->AutoSize = true;
            this->rbDemandScript->CheckAlign = System::Drawing::ContentAlignment::MiddleRight;
            this->rbDemandScript->Location = System::Drawing::Point(164, 164);
            this->rbDemandScript->Name = L"rbDemandScript";
            this->rbDemandScript->Size = System::Drawing::Size(52, 17);
            this->rbDemandScript->TabIndex = 94;
            this->rbDemandScript->Text = L"Script";
            this->rbDemandScript->UseVisualStyleBackColor = true;
            this->rbDemandScript->CheckedChanged += gcnew System::EventHandler(this, &Form1::rbDemandScript_CheckedChanged);
            // 
            // panel2
            // 
            this->panel2->Controls->Add(this->label63);
            this->panel2->Controls->Add(this->rbDemandNpc);
            this->panel2->Controls->Add(this->rbDemandPlayer);
            this->panel2->Location = System::Drawing::Point(313, 3);
            this->panel2->Name = L"panel2";
            this->panel2->Size = System::Drawing::Size(185, 26);
            this->panel2->TabIndex = 93;
            // 
            // label63
            // 
            this->label63->AutoSize = true;
            this->label63->Location = System::Drawing::Point(0, 5);
            this->label63->Name = L"label63";
            this->label63->Size = System::Drawing::Size(72, 13);
            this->label63->TabIndex = 80;
            this->label63->Text = L"Условие для";
            // 
            // rbDemandNpc
            // 
            this->rbDemandNpc->AutoSize = true;
            this->rbDemandNpc->Location = System::Drawing::Point(138, 6);
            this->rbDemandNpc->Name = L"rbDemandNpc";
            this->rbDemandNpc->Size = System::Drawing::Size(45, 17);
            this->rbDemandNpc->TabIndex = 79;
            this->rbDemandNpc->Text = L"Нпц";
            this->rbDemandNpc->UseVisualStyleBackColor = true;
            // 
            // rbDemandPlayer
            // 
            this->rbDemandPlayer->AutoSize = true;
            this->rbDemandPlayer->Checked = true;
            this->rbDemandPlayer->Location = System::Drawing::Point(71, 6);
            this->rbDemandPlayer->Name = L"rbDemandPlayer";
            this->rbDemandPlayer->Size = System::Drawing::Size(62, 17);
            this->rbDemandPlayer->TabIndex = 78;
            this->rbDemandPlayer->TabStop = true;
            this->rbDemandPlayer->Text = L"Игрока";
            this->rbDemandPlayer->UseVisualStyleBackColor = true;
            // 
            // btnDemandAdd
            // 
            this->btnDemandAdd->Location = System::Drawing::Point(504, 3);
            this->btnDemandAdd->Name = L"btnDemandAdd";
            this->btnDemandAdd->Size = System::Drawing::Size(124, 26);
            this->btnDemandAdd->TabIndex = 72;
            this->btnDemandAdd->Text = L"Добавить";
            this->btnDemandAdd->UseVisualStyleBackColor = true;
            this->btnDemandAdd->Click += gcnew System::EventHandler(this, &Form1::btnDemandAdd_Click);
            // 
            // label19
            // 
            this->label19->AutoSize = true;
            this->label19->Location = System::Drawing::Point(4, 125);
            this->label19->Name = L"label19";
            this->label19->Size = System::Drawing::Size(99, 13);
            this->label19->TabIndex = 71;
            this->label19->Text = L"Значение условия";
            // 
            // cbDemandVars
            // 
            this->cbDemandVars->DropDownHeight = 400;
            this->cbDemandVars->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbDemandVars->FormattingEnabled = true;
            this->cbDemandVars->ImeMode = System::Windows::Forms::ImeMode::NoControl;
            this->cbDemandVars->IntegralHeight = false;
            this->cbDemandVars->Location = System::Drawing::Point(76, 32);
            this->cbDemandVars->Name = L"cbDemandVars";
            this->cbDemandVars->Size = System::Drawing::Size(231, 21);
            this->cbDemandVars->TabIndex = 69;
            // 
            // cbDemandParam
            // 
            this->cbDemandParam->DropDownHeight = 400;
            this->cbDemandParam->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbDemandParam->FormattingEnabled = true;
            this->cbDemandParam->IntegralHeight = false;
            this->cbDemandParam->Location = System::Drawing::Point(76, 8);
            this->cbDemandParam->Name = L"cbDemandParam";
            this->cbDemandParam->Size = System::Drawing::Size(231, 21);
            this->cbDemandParam->TabIndex = 66;
            // 
            // rbDemandVar
            // 
            this->rbDemandVar->AutoSize = true;
            this->rbDemandVar->Location = System::Drawing::Point(4, 33);
            this->rbDemandVar->Name = L"rbDemandVar";
            this->rbDemandVar->Size = System::Drawing::Size(41, 17);
            this->rbDemandVar->TabIndex = 65;
            this->rbDemandVar->Text = L"Var";
            this->rbDemandVar->UseVisualStyleBackColor = true;
            // 
            // rbDemandParam
            // 
            this->rbDemandParam->AutoSize = true;
            this->rbDemandParam->Checked = true;
            this->rbDemandParam->Location = System::Drawing::Point(4, 8);
            this->rbDemandParam->Name = L"rbDemandParam";
            this->rbDemandParam->Size = System::Drawing::Size(73, 17);
            this->rbDemandParam->TabIndex = 62;
            this->rbDemandParam->TabStop = true;
            this->rbDemandParam->Text = L"Parameter";
            this->rbDemandParam->UseVisualStyleBackColor = true;
            // 
            // btnDemandOper
            // 
            this->btnDemandOper->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 14, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
                static_cast<System::Byte>(204)));
            this->btnDemandOper->ForeColor = System::Drawing::Color::Blue;
            this->btnDemandOper->Location = System::Drawing::Point(4, 83);
            this->btnDemandOper->Name = L"btnDemandOper";
            this->btnDemandOper->Size = System::Drawing::Size(303, 34);
            this->btnDemandOper->TabIndex = 61;
            this->btnDemandOper->Text = L">";
            this->btnDemandOper->UseVisualStyleBackColor = true;
            this->btnDemandOper->Click += gcnew System::EventHandler(this, &Form1::cbDemandOper_Click);
            // 
            // rbDemandItem
            // 
            this->rbDemandItem->AutoSize = true;
            this->rbDemandItem->Location = System::Drawing::Point(4, 57);
            this->rbDemandItem->Name = L"rbDemandItem";
            this->rbDemandItem->Size = System::Drawing::Size(45, 17);
            this->rbDemandItem->TabIndex = 57;
            this->rbDemandItem->Text = L"Item";
            this->rbDemandItem->UseVisualStyleBackColor = true;
            // 
            // tabResult
            // 
            this->tabResult->Controls->Add(this->cbResultValue);
            this->tabResult->Controls->Add(this->gbResultScript);
            this->tabResult->Controls->Add(this->cbResultItem);
            this->tabResult->Controls->Add(this->txtResultScript);
            this->tabResult->Controls->Add(this->rbResultScript);
            this->tabResult->Controls->Add(this->panel1);
            this->tabResult->Controls->Add(this->btnResultAdd);
            this->tabResult->Controls->Add(this->label41);
            this->tabResult->Controls->Add(this->numResultLock);
            this->tabResult->Controls->Add(this->radioButton8);
            this->tabResult->Controls->Add(this->label43);
            this->tabResult->Controls->Add(this->cbResultVars);
            this->tabResult->Controls->Add(this->cbResultParam);
            this->tabResult->Controls->Add(this->rbResultVar);
            this->tabResult->Controls->Add(this->rbResultParam);
            this->tabResult->Controls->Add(this->btnResultOper);
            this->tabResult->Controls->Add(this->rbResultItem);
            this->tabResult->Location = System::Drawing::Point(4, 22);
            this->tabResult->Name = L"tabResult";
            this->tabResult->Size = System::Drawing::Size(676, 199);
            this->tabResult->TabIndex = 3;
            this->tabResult->Text = L"Результат";
            this->tabResult->UseVisualStyleBackColor = true;
            // 
            // cbResultValue
            // 
            this->cbResultValue->DropDownHeight = 400;
            this->cbResultValue->FormattingEnabled = true;
            this->cbResultValue->IntegralHeight = false;
            this->cbResultValue->Location = System::Drawing::Point(127, 118);
            this->cbResultValue->Name = L"cbResultValue";
            this->cbResultValue->Size = System::Drawing::Size(180, 21);
            this->cbResultValue->TabIndex = 121;
            // 
            // gbResultScript
            // 
            this->gbResultScript->Controls->Add(this->cbResultSValue4);
            this->gbResultScript->Controls->Add(this->cbResultSValue3);
            this->gbResultScript->Controls->Add(this->cbResultSValue2);
            this->gbResultScript->Controls->Add(this->cbResultSValue1);
            this->gbResultScript->Controls->Add(this->cbResultSValue0);
            this->gbResultScript->Controls->Add(this->rbResultScriptVal0);
            this->gbResultScript->Controls->Add(this->rbResultScriptVal5);
            this->gbResultScript->Controls->Add(this->rbResultScriptVal4);
            this->gbResultScript->Controls->Add(this->rbResultScriptVal3);
            this->gbResultScript->Controls->Add(this->rbResultScriptVal2);
            this->gbResultScript->Controls->Add(this->rbResultScriptVal1);
            this->gbResultScript->Location = System::Drawing::Point(318, 26);
            this->gbResultScript->Name = L"gbResultScript";
            this->gbResultScript->Size = System::Drawing::Size(354, 134);
            this->gbResultScript->TabIndex = 108;
            this->gbResultScript->TabStop = false;
            this->gbResultScript->Text = L"Аргументы скрипта";
            this->gbResultScript->Visible = false;
            // 
            // cbResultSValue4
            // 
            this->cbResultSValue4->DropDownHeight = 400;
            this->cbResultSValue4->FormattingEnabled = true;
            this->cbResultSValue4->IntegralHeight = false;
            this->cbResultSValue4->Location = System::Drawing::Point(6, 108);
            this->cbResultSValue4->Name = L"cbResultSValue4";
            this->cbResultSValue4->Size = System::Drawing::Size(304, 21);
            this->cbResultSValue4->TabIndex = 123;
            // 
            // cbResultSValue3
            // 
            this->cbResultSValue3->DropDownHeight = 400;
            this->cbResultSValue3->FormattingEnabled = true;
            this->cbResultSValue3->IntegralHeight = false;
            this->cbResultSValue3->Location = System::Drawing::Point(6, 88);
            this->cbResultSValue3->Name = L"cbResultSValue3";
            this->cbResultSValue3->Size = System::Drawing::Size(304, 21);
            this->cbResultSValue3->TabIndex = 122;
            // 
            // cbResultSValue2
            // 
            this->cbResultSValue2->DropDownHeight = 400;
            this->cbResultSValue2->FormattingEnabled = true;
            this->cbResultSValue2->IntegralHeight = false;
            this->cbResultSValue2->Location = System::Drawing::Point(6, 68);
            this->cbResultSValue2->Name = L"cbResultSValue2";
            this->cbResultSValue2->Size = System::Drawing::Size(304, 21);
            this->cbResultSValue2->TabIndex = 121;
            // 
            // cbResultSValue1
            // 
            this->cbResultSValue1->DropDownHeight = 400;
            this->cbResultSValue1->FormattingEnabled = true;
            this->cbResultSValue1->IntegralHeight = false;
            this->cbResultSValue1->Location = System::Drawing::Point(6, 48);
            this->cbResultSValue1->Name = L"cbResultSValue1";
            this->cbResultSValue1->Size = System::Drawing::Size(304, 21);
            this->cbResultSValue1->TabIndex = 120;
            // 
            // cbResultSValue0
            // 
            this->cbResultSValue0->DropDownHeight = 400;
            this->cbResultSValue0->FormattingEnabled = true;
            this->cbResultSValue0->IntegralHeight = false;
            this->cbResultSValue0->Location = System::Drawing::Point(6, 28);
            this->cbResultSValue0->Name = L"cbResultSValue0";
            this->cbResultSValue0->Size = System::Drawing::Size(304, 21);
            this->cbResultSValue0->TabIndex = 119;
            // 
            // rbResultScriptVal0
            // 
            this->rbResultScriptVal0->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->rbResultScriptVal0->AutoSize = true;
            this->rbResultScriptVal0->Checked = true;
            this->rbResultScriptVal0->Location = System::Drawing::Point(236, 13);
            this->rbResultScriptVal0->Name = L"rbResultScriptVal0";
            this->rbResultScriptVal0->Size = System::Drawing::Size(44, 17);
            this->rbResultScriptVal0->TabIndex = 118;
            this->rbResultScriptVal0->TabStop = true;
            this->rbResultScriptVal0->Text = L"Нет";
            this->rbResultScriptVal0->UseVisualStyleBackColor = true;
            // 
            // rbResultScriptVal5
            // 
            this->rbResultScriptVal5->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->rbResultScriptVal5->AutoSize = true;
            this->rbResultScriptVal5->Location = System::Drawing::Point(316, 112);
            this->rbResultScriptVal5->Name = L"rbResultScriptVal5";
            this->rbResultScriptVal5->Size = System::Drawing::Size(31, 17);
            this->rbResultScriptVal5->TabIndex = 117;
            this->rbResultScriptVal5->Text = L"5";
            this->rbResultScriptVal5->UseVisualStyleBackColor = true;
            // 
            // rbResultScriptVal4
            // 
            this->rbResultScriptVal4->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->rbResultScriptVal4->AutoSize = true;
            this->rbResultScriptVal4->Location = System::Drawing::Point(316, 93);
            this->rbResultScriptVal4->Name = L"rbResultScriptVal4";
            this->rbResultScriptVal4->Size = System::Drawing::Size(31, 17);
            this->rbResultScriptVal4->TabIndex = 116;
            this->rbResultScriptVal4->Text = L"4";
            this->rbResultScriptVal4->UseVisualStyleBackColor = true;
            // 
            // rbResultScriptVal3
            // 
            this->rbResultScriptVal3->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->rbResultScriptVal3->AutoSize = true;
            this->rbResultScriptVal3->Location = System::Drawing::Point(316, 73);
            this->rbResultScriptVal3->Name = L"rbResultScriptVal3";
            this->rbResultScriptVal3->Size = System::Drawing::Size(31, 17);
            this->rbResultScriptVal3->TabIndex = 115;
            this->rbResultScriptVal3->Text = L"3";
            this->rbResultScriptVal3->UseVisualStyleBackColor = true;
            // 
            // rbResultScriptVal2
            // 
            this->rbResultScriptVal2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->rbResultScriptVal2->AutoSize = true;
            this->rbResultScriptVal2->Location = System::Drawing::Point(316, 52);
            this->rbResultScriptVal2->Name = L"rbResultScriptVal2";
            this->rbResultScriptVal2->Size = System::Drawing::Size(31, 17);
            this->rbResultScriptVal2->TabIndex = 114;
            this->rbResultScriptVal2->Text = L"2";
            this->rbResultScriptVal2->UseVisualStyleBackColor = true;
            // 
            // rbResultScriptVal1
            // 
            this->rbResultScriptVal1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->rbResultScriptVal1->AutoSize = true;
            this->rbResultScriptVal1->Location = System::Drawing::Point(316, 31);
            this->rbResultScriptVal1->Name = L"rbResultScriptVal1";
            this->rbResultScriptVal1->Size = System::Drawing::Size(31, 17);
            this->rbResultScriptVal1->TabIndex = 113;
            this->rbResultScriptVal1->Text = L"1";
            this->rbResultScriptVal1->UseVisualStyleBackColor = true;
            this->rbResultScriptVal1->CheckedChanged += gcnew System::EventHandler(this, &Form1::radioButton7_CheckedChanged);
            // 
            // cbResultItem
            // 
            this->cbResultItem->DropDownHeight = 400;
            this->cbResultItem->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbResultItem->FormattingEnabled = true;
            this->cbResultItem->IntegralHeight = false;
            this->cbResultItem->Location = System::Drawing::Point(76, 55);
            this->cbResultItem->Name = L"cbResultItem";
            this->cbResultItem->Size = System::Drawing::Size(231, 21);
            this->cbResultItem->Sorted = true;
            this->cbResultItem->TabIndex = 98;
            // 
            // txtResultScript
            // 
            this->txtResultScript->Location = System::Drawing::Point(350, 163);
            this->txtResultScript->Name = L"txtResultScript";
            this->txtResultScript->Size = System::Drawing::Size(313, 20);
            this->txtResultScript->TabIndex = 97;
            // 
            // rbResultScript
            // 
            this->rbResultScript->AutoSize = true;
            this->rbResultScript->CheckAlign = System::Drawing::ContentAlignment::MiddleRight;
            this->rbResultScript->Location = System::Drawing::Point(181, 166);
            this->rbResultScript->Name = L"rbResultScript";
            this->rbResultScript->Size = System::Drawing::Size(61, 17);
            this->rbResultScript->TabIndex = 96;
            this->rbResultScript->Text = L"Скрипт";
            this->rbResultScript->UseVisualStyleBackColor = true;
            this->rbResultScript->CheckedChanged += gcnew System::EventHandler(this, &Form1::rbResultScript_CheckedChanged);
            // 
            // panel1
            // 
            this->panel1->Controls->Add(this->label62);
            this->panel1->Controls->Add(this->rbResultNpc);
            this->panel1->Controls->Add(this->rbResultPlayer);
            this->panel1->Location = System::Drawing::Point(313, 3);
            this->panel1->Name = L"panel1";
            this->panel1->Size = System::Drawing::Size(187, 26);
            this->panel1->TabIndex = 92;
            // 
            // label62
            // 
            this->label62->AutoSize = true;
            this->label62->Location = System::Drawing::Point(2, 5);
            this->label62->Name = L"label62";
            this->label62->Size = System::Drawing::Size(80, 13);
            this->label62->TabIndex = 80;
            this->label62->Text = L"Результат для";
            // 
            // rbResultNpc
            // 
            this->rbResultNpc->AutoSize = true;
            this->rbResultNpc->Location = System::Drawing::Point(143, 3);
            this->rbResultNpc->Name = L"rbResultNpc";
            this->rbResultNpc->Size = System::Drawing::Size(45, 17);
            this->rbResultNpc->TabIndex = 79;
            this->rbResultNpc->Text = L"Нпц";
            this->rbResultNpc->UseVisualStyleBackColor = true;
            // 
            // rbResultPlayer
            // 
            this->rbResultPlayer->AutoSize = true;
            this->rbResultPlayer->Checked = true;
            this->rbResultPlayer->Location = System::Drawing::Point(85, 3);
            this->rbResultPlayer->Name = L"rbResultPlayer";
            this->rbResultPlayer->Size = System::Drawing::Size(62, 17);
            this->rbResultPlayer->TabIndex = 78;
            this->rbResultPlayer->TabStop = true;
            this->rbResultPlayer->Text = L"Игрока";
            this->rbResultPlayer->UseVisualStyleBackColor = true;
            // 
            // btnResultAdd
            // 
            this->btnResultAdd->Location = System::Drawing::Point(506, 3);
            this->btnResultAdd->Name = L"btnResultAdd";
            this->btnResultAdd->Size = System::Drawing::Size(166, 23);
            this->btnResultAdd->TabIndex = 90;
            this->btnResultAdd->Text = L"Добавить";
            this->btnResultAdd->UseVisualStyleBackColor = true;
            this->btnResultAdd->Click += gcnew System::EventHandler(this, &Form1::btnResultAdd_Click);
            // 
            // label41
            // 
            this->label41->AutoSize = true;
            this->label41->Location = System::Drawing::Point(264, 153);
            this->label41->Name = L"label41";
            this->label41->Size = System::Drawing::Size(30, 13);
            this->label41->TabIndex = 89;
            this->label41->Text = L"мин.";
            this->label41->Visible = false;
            // 
            // numResultLock
            // 
            this->numResultLock->Location = System::Drawing::Point(149, 146);
            this->numResultLock->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {10000000, 0, 0, 0});
            this->numResultLock->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) {1, 0, 0, 0});
            this->numResultLock->Name = L"numResultLock";
            this->numResultLock->Size = System::Drawing::Size(109, 20);
            this->numResultLock->TabIndex = 88;
            this->numResultLock->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {60, 0, 0, 0});
            this->numResultLock->Visible = false;
            // 
            // radioButton8
            // 
            this->radioButton8->AutoSize = true;
            this->radioButton8->Location = System::Drawing::Point(4, 146);
            this->radioButton8->Name = L"radioButton8";
            this->radioButton8->Size = System::Drawing::Size(138, 17);
            this->radioButton8->TabIndex = 87;
            this->radioButton8->Text = L"Блокировка ответа на";
            this->radioButton8->UseVisualStyleBackColor = true;
            this->radioButton8->Visible = false;
            // 
            // label43
            // 
            this->label43->AutoSize = true;
            this->label43->Location = System::Drawing::Point(4, 123);
            this->label43->Name = L"label43";
            this->label43->Size = System::Drawing::Size(115, 13);
            this->label43->TabIndex = 86;
            this->label43->Text = L"Значение результата";
            // 
            // cbResultVars
            // 
            this->cbResultVars->DropDownHeight = 400;
            this->cbResultVars->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbResultVars->FormattingEnabled = true;
            this->cbResultVars->ImeMode = System::Windows::Forms::ImeMode::NoControl;
            this->cbResultVars->IntegralHeight = false;
            this->cbResultVars->Location = System::Drawing::Point(76, 31);
            this->cbResultVars->Name = L"cbResultVars";
            this->cbResultVars->Size = System::Drawing::Size(231, 21);
            this->cbResultVars->TabIndex = 84;
            // 
            // cbResultParam
            // 
            this->cbResultParam->DropDownHeight = 400;
            this->cbResultParam->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbResultParam->FormattingEnabled = true;
            this->cbResultParam->IntegralHeight = false;
            this->cbResultParam->Location = System::Drawing::Point(76, 8);
            this->cbResultParam->Name = L"cbResultParam";
            this->cbResultParam->Size = System::Drawing::Size(231, 21);
            this->cbResultParam->TabIndex = 83;
            // 
            // rbResultVar
            // 
            this->rbResultVar->AutoSize = true;
            this->rbResultVar->Location = System::Drawing::Point(4, 32);
            this->rbResultVar->Name = L"rbResultVar";
            this->rbResultVar->Size = System::Drawing::Size(41, 17);
            this->rbResultVar->TabIndex = 80;
            this->rbResultVar->Text = L"Var";
            this->rbResultVar->UseVisualStyleBackColor = true;
            // 
            // rbResultParam
            // 
            this->rbResultParam->AutoSize = true;
            this->rbResultParam->Location = System::Drawing::Point(4, 9);
            this->rbResultParam->Name = L"rbResultParam";
            this->rbResultParam->Size = System::Drawing::Size(73, 17);
            this->rbResultParam->TabIndex = 79;
            this->rbResultParam->Text = L"Parameter";
            this->rbResultParam->UseVisualStyleBackColor = true;
            // 
            // btnResultOper
            // 
            this->btnResultOper->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 14, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
                static_cast<System::Byte>(204)));
            this->btnResultOper->ForeColor = System::Drawing::Color::Red;
            this->btnResultOper->Location = System::Drawing::Point(4, 82);
            this->btnResultOper->Name = L"btnResultOper";
            this->btnResultOper->Size = System::Drawing::Size(303, 34);
            this->btnResultOper->TabIndex = 76;
            this->btnResultOper->Text = L"+";
            this->btnResultOper->UseVisualStyleBackColor = true;
            this->btnResultOper->Click += gcnew System::EventHandler(this, &Form1::btnResultOper_Click);
            // 
            // rbResultItem
            // 
            this->rbResultItem->AutoSize = true;
            this->rbResultItem->Location = System::Drawing::Point(4, 56);
            this->rbResultItem->Name = L"rbResultItem";
            this->rbResultItem->Size = System::Drawing::Size(45, 17);
            this->rbResultItem->TabIndex = 72;
            this->rbResultItem->Text = L"Item";
            this->rbResultItem->UseVisualStyleBackColor = true;
            // 
            // tabText
            // 
            this->tabText->Controls->Add(this->gridText);
            this->tabText->Location = System::Drawing::Point(4, 22);
            this->tabText->Name = L"tabText";
            this->tabText->Size = System::Drawing::Size(676, 199);
            this->tabText->TabIndex = 7;
            this->tabText->Text = L"Строки";
            this->tabText->UseVisualStyleBackColor = true;
            // 
            // gridText
            // 
            this->gridText->AutoSizeColumnsMode = System::Windows::Forms::DataGridViewAutoSizeColumnsMode::Fill;
            this->gridText->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
            this->gridText->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(2) {this->gNumber, this->gText});
            this->gridText->Dock = System::Windows::Forms::DockStyle::Fill;
            this->gridText->Location = System::Drawing::Point(0, 0);
            this->gridText->Name = L"gridText";
            this->gridText->Size = System::Drawing::Size(676, 199);
            this->gridText->TabIndex = 0;
            this->gridText->CellContentClick += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &Form1::dataGridView1_CellContentClick);
            this->gridText->CellEndEdit += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &Form1::gridText_CellEndEdit);
            this->gridText->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &Form1::gridText_KeyDown);
            this->gridText->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &Form1::gridText_KeyPress);
            this->gridText->KeyUp += gcnew System::Windows::Forms::KeyEventHandler(this, &Form1::gridText_KeyUp);
            // 
            // gNumber
            // 
            dataGridViewCellStyle1->NullValue = nullptr;
            this->gNumber->DefaultCellStyle = dataGridViewCellStyle1;
            this->gNumber->FillWeight = 18.27411F;
            this->gNumber->HeaderText = L"Number";
            this->gNumber->Name = L"gNumber";
            // 
            // gText
            // 
            this->gText->FillWeight = 181.7259F;
            this->gText->HeaderText = L"Text                                                    Refresh - F5    Copy - F6" 
                L"";
            this->gText->Name = L"gText";
            // 
            // tabComment
            // 
            this->tabComment->Controls->Add(this->rtComment);
            this->tabComment->Location = System::Drawing::Point(4, 22);
            this->tabComment->Name = L"tabComment";
            this->tabComment->Size = System::Drawing::Size(676, 199);
            this->tabComment->TabIndex = 6;
            this->tabComment->Text = L"Комментарий";
            this->tabComment->UseVisualStyleBackColor = true;
            // 
            // rtComment
            // 
            this->rtComment->Dock = System::Windows::Forms::DockStyle::Fill;
            this->rtComment->Location = System::Drawing::Point(0, 0);
            this->rtComment->Name = L"rtComment";
            this->rtComment->Size = System::Drawing::Size(676, 199);
            this->rtComment->TabIndex = 0;
            this->rtComment->Text = L"";
            // 
            // tabVars
            // 
            this->tabVars->Controls->Add(this->btnVarDelete);
            this->tabVars->Controls->Add(this->btnVarAdd);
            this->tabVars->Controls->Add(this->btnVarChange);
            this->tabVars->Controls->Add(this->groupBox2);
            this->tabVars->Controls->Add(this->groupBox8);
            this->tabVars->Controls->Add(this->groupBox3);
            this->tabVars->Location = System::Drawing::Point(4, 25);
            this->tabVars->Name = L"tabVars";
            this->tabVars->Size = System::Drawing::Size(684, 441);
            this->tabVars->TabIndex = 12;
            this->tabVars->Text = L"Редактор переменных";
            this->tabVars->UseVisualStyleBackColor = true;
            // 
            // btnVarDelete
            // 
            this->btnVarDelete->Location = System::Drawing::Point(584, 303);
            this->btnVarDelete->Name = L"btnVarDelete";
            this->btnVarDelete->Size = System::Drawing::Size(86, 46);
            this->btnVarDelete->TabIndex = 31;
            this->btnVarDelete->Text = L"Удалить (по номеру)";
            this->btnVarDelete->UseVisualStyleBackColor = true;
            this->btnVarDelete->Click += gcnew System::EventHandler(this, &Form1::btnVarDelete_Click);
            // 
            // btnVarAdd
            // 
            this->btnVarAdd->Location = System::Drawing::Point(584, 200);
            this->btnVarAdd->Name = L"btnVarAdd";
            this->btnVarAdd->Size = System::Drawing::Size(86, 45);
            this->btnVarAdd->TabIndex = 30;
            this->btnVarAdd->Text = L"Добавить";
            this->btnVarAdd->UseVisualStyleBackColor = true;
            this->btnVarAdd->Click += gcnew System::EventHandler(this, &Form1::btnVarAdd_Click);
            // 
            // btnVarChange
            // 
            this->btnVarChange->Location = System::Drawing::Point(584, 251);
            this->btnVarChange->Name = L"btnVarChange";
            this->btnVarChange->Size = System::Drawing::Size(86, 46);
            this->btnVarChange->TabIndex = 32;
            this->btnVarChange->Text = L"Изменить";
            this->btnVarChange->UseVisualStyleBackColor = true;
            this->btnVarChange->Click += gcnew System::EventHandler(this, &Form1::btnVarChange_Click);
            // 
            // groupBox2
            // 
            this->groupBox2->Controls->Add(this->lbAllVars);
            this->groupBox2->Location = System::Drawing::Point(5, 3);
            this->groupBox2->Name = L"groupBox2";
            this->groupBox2->Size = System::Drawing::Size(671, 188);
            this->groupBox2->TabIndex = 35;
            this->groupBox2->TabStop = false;
            this->groupBox2->Text = L"Списки переменных";
            // 
            // lbAllVars
            // 
            this->lbAllVars->Dock = System::Windows::Forms::DockStyle::Fill;
            this->lbAllVars->FormattingEnabled = true;
            this->lbAllVars->HorizontalScrollbar = true;
            this->lbAllVars->Location = System::Drawing::Point(3, 16);
            this->lbAllVars->Name = L"lbAllVars";
            this->lbAllVars->Size = System::Drawing::Size(665, 169);
            this->lbAllVars->TabIndex = 0;
            this->lbAllVars->MouseDoubleClick += gcnew System::Windows::Forms::MouseEventHandler(this, &Form1::lbVars_MouseDoubleClick);
            // 
            // groupBox8
            // 
            this->groupBox8->Controls->Add(this->rbVarLocalItem);
            this->groupBox8->Controls->Add(this->cbVarNoCheck);
            this->groupBox8->Controls->Add(this->rbVarLocalMap);
            this->groupBox8->Controls->Add(this->rbVarLocalLocation);
            this->groupBox8->Controls->Add(this->cbVarRandom);
            this->groupBox8->Controls->Add(this->cbVarQuest);
            this->groupBox8->Controls->Add(this->rbVarUnicum);
            this->groupBox8->Controls->Add(this->rbVarGlobal);
            this->groupBox8->Controls->Add(this->rbVarLocal);
            this->groupBox8->Location = System::Drawing::Point(376, 196);
            this->groupBox8->Name = L"groupBox8";
            this->groupBox8->Size = System::Drawing::Size(203, 224);
            this->groupBox8->TabIndex = 34;
            this->groupBox8->TabStop = false;
            this->groupBox8->Text = L"Флаги";
            // 
            // rbVarLocalItem
            // 
            this->rbVarLocalItem->AutoSize = true;
            this->rbVarLocalItem->Location = System::Drawing::Point(6, 99);
            this->rbVarLocalItem->Name = L"rbVarLocalItem";
            this->rbVarLocalItem->Size = System::Drawing::Size(133, 17);
            this->rbVarLocalItem->TabIndex = 41;
            this->rbVarLocalItem->Text = L"Локальная предмета";
            this->rbVarLocalItem->UseVisualStyleBackColor = true;
            // 
            // cbVarNoCheck
            // 
            this->cbVarNoCheck->AutoSize = true;
            this->cbVarNoCheck->Location = System::Drawing::Point(6, 157);
            this->cbVarNoCheck->Name = L"cbVarNoCheck";
            this->cbVarNoCheck->Size = System::Drawing::Size(83, 17);
            this->cbVarNoCheck->TabIndex = 2;
            this->cbVarNoCheck->Text = L"Без границ";
            this->cbVarNoCheck->UseVisualStyleBackColor = true;
            // 
            // rbVarLocalMap
            // 
            this->rbVarLocalMap->AutoSize = true;
            this->rbVarLocalMap->Location = System::Drawing::Point(6, 83);
            this->rbVarLocalMap->Name = L"rbVarLocalMap";
            this->rbVarLocalMap->Size = System::Drawing::Size(115, 17);
            this->rbVarLocalMap->TabIndex = 39;
            this->rbVarLocalMap->Text = L"Локальная карты";
            this->rbVarLocalMap->UseVisualStyleBackColor = true;
            // 
            // rbVarLocalLocation
            // 
            this->rbVarLocalLocation->AutoSize = true;
            this->rbVarLocalLocation->Location = System::Drawing::Point(6, 67);
            this->rbVarLocalLocation->Name = L"rbVarLocalLocation";
            this->rbVarLocalLocation->Size = System::Drawing::Size(126, 17);
            this->rbVarLocalLocation->TabIndex = 40;
            this->rbVarLocalLocation->Text = L"Локальная локации";
            this->rbVarLocalLocation->UseVisualStyleBackColor = true;
            // 
            // cbVarRandom
            // 
            this->cbVarRandom->AutoSize = true;
            this->cbVarRandom->Location = System::Drawing::Point(6, 140);
            this->cbVarRandom->Name = L"cbVarRandom";
            this->cbVarRandom->Size = System::Drawing::Size(83, 17);
            this->cbVarRandom->TabIndex = 1;
            this->cbVarRandom->Text = L"Рандомная";
            this->cbVarRandom->UseVisualStyleBackColor = true;
            // 
            // cbVarQuest
            // 
            this->cbVarQuest->AutoSize = true;
            this->cbVarQuest->Location = System::Drawing::Point(6, 123);
            this->cbVarQuest->Name = L"cbVarQuest";
            this->cbVarQuest->Size = System::Drawing::Size(80, 17);
            this->cbVarQuest->TabIndex = 0;
            this->cbVarQuest->Text = L"Квестовая";
            this->cbVarQuest->UseVisualStyleBackColor = true;
            // 
            // rbVarUnicum
            // 
            this->rbVarUnicum->AutoSize = true;
            this->rbVarUnicum->Location = System::Drawing::Point(6, 51);
            this->rbVarUnicum->Name = L"rbVarUnicum";
            this->rbVarUnicum->Size = System::Drawing::Size(87, 17);
            this->rbVarUnicum->TabIndex = 38;
            this->rbVarUnicum->Text = L"Уникальная";
            this->rbVarUnicum->UseVisualStyleBackColor = true;
            // 
            // rbVarGlobal
            // 
            this->rbVarGlobal->AutoSize = true;
            this->rbVarGlobal->Checked = true;
            this->rbVarGlobal->Location = System::Drawing::Point(6, 19);
            this->rbVarGlobal->Name = L"rbVarGlobal";
            this->rbVarGlobal->Size = System::Drawing::Size(85, 17);
            this->rbVarGlobal->TabIndex = 36;
            this->rbVarGlobal->TabStop = true;
            this->rbVarGlobal->Text = L"Глобальная";
            this->rbVarGlobal->UseVisualStyleBackColor = true;
            // 
            // rbVarLocal
            // 
            this->rbVarLocal->AutoSize = true;
            this->rbVarLocal->Location = System::Drawing::Point(6, 35);
            this->rbVarLocal->Name = L"rbVarLocal";
            this->rbVarLocal->Size = System::Drawing::Size(81, 17);
            this->rbVarLocal->TabIndex = 37;
            this->rbVarLocal->Text = L"Локальная";
            this->rbVarLocal->UseVisualStyleBackColor = true;
            // 
            // groupBox3
            // 
            this->groupBox3->Controls->Add(this->numVarMax);
            this->groupBox3->Controls->Add(this->numVarId);
            this->groupBox3->Controls->Add(this->numVarMin);
            this->groupBox3->Controls->Add(this->txtVarDesc);
            this->groupBox3->Controls->Add(this->numVarStart);
            this->groupBox3->Controls->Add(this->txtVarName);
            this->groupBox3->Controls->Add(this->label20);
            this->groupBox3->Controls->Add(this->label23);
            this->groupBox3->Controls->Add(this->label25);
            this->groupBox3->Controls->Add(this->label37);
            this->groupBox3->Controls->Add(this->label38);
            this->groupBox3->Location = System::Drawing::Point(5, 197);
            this->groupBox3->Name = L"groupBox3";
            this->groupBox3->Size = System::Drawing::Size(365, 223);
            this->groupBox3->TabIndex = 33;
            this->groupBox3->TabStop = false;
            this->groupBox3->Text = L"Описание параметра";
            // 
            // numVarMax
            // 
            this->numVarMax->Location = System::Drawing::Point(6, 74);
            this->numVarMax->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {2147483000, 0, 0, 0});
            this->numVarMax->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) {2147483000, 0, 0, System::Int32::MinValue});
            this->numVarMax->Name = L"numVarMax";
            this->numVarMax->Size = System::Drawing::Size(84, 20);
            this->numVarMax->TabIndex = 35;
            this->numVarMax->ValueChanged += gcnew System::EventHandler(this, &Form1::numVarMax_ValueChanged);
            // 
            // numVarId
            // 
            this->numVarId->Location = System::Drawing::Point(292, 32);
            this->numVarId->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {65565, 0, 0, 0});
            this->numVarId->Name = L"numVarId";
            this->numVarId->Size = System::Drawing::Size(66, 20);
            this->numVarId->TabIndex = 33;
            this->numVarId->ValueChanged += gcnew System::EventHandler(this, &Form1::numVarId_ValueChanged);
            // 
            // numVarMin
            // 
            this->numVarMin->Location = System::Drawing::Point(6, 113);
            this->numVarMin->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {2147483000, 0, 0, 0});
            this->numVarMin->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) {2147483000, 0, 0, System::Int32::MinValue});
            this->numVarMin->Name = L"numVarMin";
            this->numVarMin->Size = System::Drawing::Size(84, 20);
            this->numVarMin->TabIndex = 34;
            this->numVarMin->ValueChanged += gcnew System::EventHandler(this, &Form1::numVarMin_ValueChanged);
            // 
            // txtVarDesc
            // 
            this->txtVarDesc->Location = System::Drawing::Point(99, 55);
            this->txtVarDesc->MaxLength = 1024;
            this->txtVarDesc->Name = L"txtVarDesc";
            this->txtVarDesc->Size = System::Drawing::Size(259, 162);
            this->txtVarDesc->TabIndex = 17;
            this->txtVarDesc->Text = L"";
            this->txtVarDesc->TextChanged += gcnew System::EventHandler(this, &Form1::txtVarDesc_TextChanged);
            // 
            // numVarStart
            // 
            this->numVarStart->Location = System::Drawing::Point(6, 32);
            this->numVarStart->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {2147483000, 0, 0, 0});
            this->numVarStart->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) {2147483000, 0, 0, System::Int32::MinValue});
            this->numVarStart->Name = L"numVarStart";
            this->numVarStart->Size = System::Drawing::Size(84, 20);
            this->numVarStart->TabIndex = 33;
            this->numVarStart->ValueChanged += gcnew System::EventHandler(this, &Form1::numVarStart_ValueChanged);
            // 
            // txtVarName
            // 
            this->txtVarName->Location = System::Drawing::Point(99, 32);
            this->txtVarName->MaxLength = 256;
            this->txtVarName->Name = L"txtVarName";
            this->txtVarName->Size = System::Drawing::Size(187, 20);
            this->txtVarName->TabIndex = 31;
            this->txtVarName->TextChanged += gcnew System::EventHandler(this, &Form1::txtVarName_TextChanged);
            // 
            // label20
            // 
            this->label20->AutoSize = true;
            this->label20->Location = System::Drawing::Point(6, 58);
            this->label20->Name = L"label20";
            this->label20->Size = System::Drawing::Size(87, 13);
            this->label20->TabIndex = 32;
            this->label20->Text = L"Макс. значение";
            // 
            // label23
            // 
            this->label23->AutoSize = true;
            this->label23->Location = System::Drawing::Point(287, 16);
            this->label23->Name = L"label23";
            this->label23->Size = System::Drawing::Size(41, 13);
            this->label23->TabIndex = 32;
            this->label23->Text = L"Номер";
            // 
            // label25
            // 
            this->label25->AutoSize = true;
            this->label25->Location = System::Drawing::Point(6, 97);
            this->label25->Name = L"label25";
            this->label25->Size = System::Drawing::Size(81, 13);
            this->label25->TabIndex = 31;
            this->label25->Text = L"Мин. значение";
            // 
            // label37
            // 
            this->label37->AutoSize = true;
            this->label37->Location = System::Drawing::Point(105, 16);
            this->label37->Name = L"label37";
            this->label37->Size = System::Drawing::Size(89, 13);
            this->label37->TabIndex = 30;
            this->label37->Text = L"Имя и описание";
            // 
            // label38
            // 
            this->label38->AutoSize = true;
            this->label38->Location = System::Drawing::Point(6, 16);
            this->label38->Name = L"label38";
            this->label38->Size = System::Drawing::Size(78, 13);
            this->label38->TabIndex = 30;
            this->label38->Text = L"Стартовое зн.";
            // 
            // tbMain
            // 
            this->tbMain->Controls->Add(this->rbEnglish);
            this->tbMain->Controls->Add(this->rbRussian);
            this->tbMain->Controls->Add(this->label64);
            this->tbMain->Location = System::Drawing::Point(4, 25);
            this->tbMain->Name = L"tbMain";
            this->tbMain->Size = System::Drawing::Size(684, 441);
            this->tbMain->TabIndex = 14;
            this->tbMain->Text = L"Options";
            this->tbMain->UseVisualStyleBackColor = true;
            // 
            // rbEnglish
            // 
            this->rbEnglish->AutoSize = true;
            this->rbEnglish->Location = System::Drawing::Point(23, 55);
            this->rbEnglish->Name = L"rbEnglish";
            this->rbEnglish->Size = System::Drawing::Size(59, 17);
            this->rbEnglish->TabIndex = 2;
            this->rbEnglish->Text = L"English";
            this->rbEnglish->UseVisualStyleBackColor = true;
            this->rbEnglish->CheckedChanged += gcnew System::EventHandler(this, &Form1::rbRussian_CheckedChanged);
            // 
            // rbRussian
            // 
            this->rbRussian->AutoSize = true;
            this->rbRussian->Checked = true;
            this->rbRussian->Location = System::Drawing::Point(23, 32);
            this->rbRussian->Name = L"rbRussian";
            this->rbRussian->Size = System::Drawing::Size(67, 17);
            this->rbRussian->TabIndex = 1;
            this->rbRussian->TabStop = true;
            this->rbRussian->Text = L"Русский";
            this->rbRussian->UseVisualStyleBackColor = true;
            this->rbRussian->CheckedChanged += gcnew System::EventHandler(this, &Form1::rbRussian_CheckedChanged);
            // 
            // label64
            // 
            this->label64->AutoSize = true;
            this->label64->Location = System::Drawing::Point(20, 16);
            this->label64->Name = L"label64";
            this->label64->Size = System::Drawing::Size(94, 13);
            this->label64->TabIndex = 0;
            this->label64->Text = L"Язык / Language";
            // 
            // btnUpLine
            // 
            this->btnUpLine->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
            this->btnUpLine->Enabled = false;
            this->btnUpLine->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 14, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
                static_cast<System::Byte>(204)));
            this->btnUpLine->Location = System::Drawing::Point(497, 229);
            this->btnUpLine->Name = L"btnUpLine";
            this->btnUpLine->Size = System::Drawing::Size(38, 28);
            this->btnUpLine->TabIndex = 4;
            this->btnUpLine->Text = L"/\\";
            this->btnUpLine->UseVisualStyleBackColor = true;
            this->btnUpLine->Click += gcnew System::EventHandler(this, &Form1::btnUpLine_Click);
            // 
            // btnDeleteLine
            // 
            this->btnDeleteLine->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
            this->btnDeleteLine->Enabled = false;
            this->btnDeleteLine->Location = System::Drawing::Point(447, 262);
            this->btnDeleteLine->Name = L"btnDeleteLine";
            this->btnDeleteLine->Size = System::Drawing::Size(108, 31);
            this->btnDeleteLine->TabIndex = 5;
            this->btnDeleteLine->Text = L"Удалить строку";
            this->btnDeleteLine->UseVisualStyleBackColor = true;
            this->btnDeleteLine->Click += gcnew System::EventHandler(this, &Form1::btnDeleteLine_Click);
            // 
            // btnDownLine
            // 
            this->btnDownLine->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
            this->btnDownLine->Enabled = false;
            this->btnDownLine->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 14, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
                static_cast<System::Byte>(204)));
            this->btnDownLine->Location = System::Drawing::Point(497, 297);
            this->btnDownLine->Name = L"btnDownLine";
            this->btnDownLine->Size = System::Drawing::Size(38, 28);
            this->btnDownLine->TabIndex = 6;
            this->btnDownLine->Text = L"\\/";
            this->btnDownLine->UseVisualStyleBackColor = true;
            this->btnDownLine->Click += gcnew System::EventHandler(this, &Form1::btnDownLine_Click);
            // 
            // radioButton1
            // 
            this->radioButton1->AutoSize = true;
            this->radioButton1->Location = System::Drawing::Point(6, 132);
            this->radioButton1->Name = L"radioButton1";
            this->radioButton1->Size = System::Drawing::Size(58, 17);
            this->radioButton1->TabIndex = 29;
            this->radioButton1->TabStop = true;
            this->radioButton1->Text = L"Перки";
            this->radioButton1->UseVisualStyleBackColor = true;
            // 
            // btnSkill
            // 
            this->btnSkill->AutoSize = true;
            this->btnSkill->Location = System::Drawing::Point(6, 109);
            this->btnSkill->Name = L"btnSkill";
            this->btnSkill->Size = System::Drawing::Size(59, 17);
            this->btnSkill->TabIndex = 28;
            this->btnSkill->TabStop = true;
            this->btnSkill->Text = L"Скилы";
            this->btnSkill->UseVisualStyleBackColor = true;
            // 
            // btnStat
            // 
            this->btnStat->AutoSize = true;
            this->btnStat->Location = System::Drawing::Point(6, 86);
            this->btnStat->Name = L"btnStat";
            this->btnStat->Size = System::Drawing::Size(57, 17);
            this->btnStat->TabIndex = 27;
            this->btnStat->TabStop = true;
            this->btnStat->Text = L"Статы";
            this->btnStat->UseVisualStyleBackColor = true;
            // 
            // localBtn
            // 
            this->localBtn->AutoSize = true;
            this->localBtn->Checked = true;
            this->localBtn->Location = System::Drawing::Point(6, 41);
            this->localBtn->Name = L"localBtn";
            this->localBtn->Size = System::Drawing::Size(126, 17);
            this->localBtn->TabIndex = 23;
            this->localBtn->TabStop = true;
            this->localBtn->Text = L"Локал. переменная";
            this->localBtn->UseVisualStyleBackColor = true;
            // 
            // playerBtn
            // 
            this->playerBtn->AutoSize = true;
            this->playerBtn->Location = System::Drawing::Point(6, 63);
            this->playerBtn->Name = L"playerBtn";
            this->playerBtn->Size = System::Drawing::Size(128, 17);
            this->playerBtn->TabIndex = 26;
            this->playerBtn->TabStop = true;
            this->playerBtn->Text = L"Переменная игрока";
            this->playerBtn->UseVisualStyleBackColor = true;
            // 
            // globalBtn
            // 
            this->globalBtn->AutoSize = true;
            this->globalBtn->Location = System::Drawing::Point(6, 19);
            this->globalBtn->Name = L"globalBtn";
            this->globalBtn->Size = System::Drawing::Size(118, 17);
            this->globalBtn->TabIndex = 24;
            this->globalBtn->TabStop = true;
            this->globalBtn->Text = L"Глоб. переменная";
            this->globalBtn->UseVisualStyleBackColor = true;
            // 
            // label31
            // 
            this->label31->AutoSize = true;
            this->label31->Location = System::Drawing::Point(159, 108);
            this->label31->Name = L"label31";
            this->label31->Size = System::Drawing::Size(87, 13);
            this->label31->TabIndex = 32;
            this->label31->Text = L"Макс. значение";
            // 
            // label29
            // 
            this->label29->AutoSize = true;
            this->label29->Location = System::Drawing::Point(6, 63);
            this->label29->Name = L"label29";
            this->label29->Size = System::Drawing::Size(99, 13);
            this->label29->TabIndex = 32;
            this->label29->Text = L"Номер параметра";
            // 
            // label33
            // 
            this->label33->AutoSize = true;
            this->label33->Location = System::Drawing::Point(158, 65);
            this->label33->Name = L"label33";
            this->label33->Size = System::Drawing::Size(81, 13);
            this->label33->TabIndex = 31;
            this->label33->Text = L"Мин. значение";
            // 
            // lbIntro
            // 
            this->lbIntro->AutoSize = true;
            this->lbIntro->Location = System::Drawing::Point(6, 21);
            this->lbIntro->Name = L"lbIntro";
            this->lbIntro->Size = System::Drawing::Size(87, 13);
            this->lbIntro->TabIndex = 30;
            this->lbIntro->Text = L"Имя параметра";
            // 
            // label34
            // 
            this->label34->AutoSize = true;
            this->label34->Location = System::Drawing::Point(158, 21);
            this->label34->Name = L"label34";
            this->label34->Size = System::Drawing::Size(55, 13);
            this->label34->TabIndex = 30;
            this->label34->Text = L"Значение";
            // 
            // lbVars
            // 
            this->lbVars->FormattingEnabled = true;
            this->lbVars->HorizontalScrollbar = true;
            this->lbVars->Location = System::Drawing::Point(6, 19);
            this->lbVars->Name = L"lbVars";
            this->lbVars->Size = System::Drawing::Size(135, 160);
            this->lbVars->TabIndex = 0;
            // 
            // btnUslRezDelete
            // 
            this->btnUslRezDelete->Location = System::Drawing::Point(5, 165);
            this->btnUslRezDelete->Name = L"btnUslRezDelete";
            this->btnUslRezDelete->Size = System::Drawing::Size(87, 23);
            this->btnUslRezDelete->TabIndex = 24;
            this->btnUslRezDelete->Text = L"Удалить";
            this->btnUslRezDelete->UseVisualStyleBackColor = true;
            // 
            // rbRez
            // 
            this->rbRez->AutoSize = true;
            this->rbRez->Location = System::Drawing::Point(6, 39);
            this->rbRez->Name = L"rbRez";
            this->rbRez->Size = System::Drawing::Size(78, 17);
            this->rbRez->TabIndex = 23;
            this->rbRez->Text = L"Результат";
            this->rbRez->UseVisualStyleBackColor = true;
            // 
            // rbUsl
            // 
            this->rbUsl->AutoSize = true;
            this->rbUsl->Checked = true;
            this->rbUsl->Location = System::Drawing::Point(6, 19);
            this->rbUsl->Name = L"rbUsl";
            this->rbUsl->Size = System::Drawing::Size(70, 17);
            this->rbUsl->TabIndex = 22;
            this->rbUsl->TabStop = true;
            this->rbUsl->Text = L"Условие";
            this->rbUsl->UseVisualStyleBackColor = true;
            // 
            // btnUslRezChange
            // 
            this->btnUslRezChange->Location = System::Drawing::Point(5, 136);
            this->btnUslRezChange->Name = L"btnUslRezChange";
            this->btnUslRezChange->Size = System::Drawing::Size(87, 23);
            this->btnUslRezChange->TabIndex = 21;
            this->btnUslRezChange->Text = L"Изменить";
            this->btnUslRezChange->UseVisualStyleBackColor = true;
            // 
            // btnUslRezAdd
            // 
            this->btnUslRezAdd->Location = System::Drawing::Point(5, 108);
            this->btnUslRezAdd->Name = L"btnUslRezAdd";
            this->btnUslRezAdd->Size = System::Drawing::Size(87, 23);
            this->btnUslRezAdd->TabIndex = 20;
            this->btnUslRezAdd->Text = L"Добавить";
            this->btnUslRezAdd->UseVisualStyleBackColor = true;
            // 
            // label22
            // 
            this->label22->AutoSize = true;
            this->label22->Location = System::Drawing::Point(352, 37);
            this->label22->Name = L"label22";
            this->label22->Size = System::Drawing::Size(161, 13);
            this->label22->TabIndex = 56;
            this->label22->Text = L"Значение условия/результата";
            // 
            // cbVarType
            // 
            this->cbVarType->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbVarType->FormattingEnabled = true;
            this->cbVarType->Items->AddRange(gcnew cli::array< System::Object^  >(3) {L"Global", L"Local", L"Player"});
            this->cbVarType->Location = System::Drawing::Point(77, 85);
            this->cbVarType->Name = L"cbVarType";
            this->cbVarType->Size = System::Drawing::Size(70, 21);
            this->cbVarType->TabIndex = 55;
            // 
            // cbVar_usl
            // 
            this->cbVar_usl->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbVar_usl->FormattingEnabled = true;
            this->cbVar_usl->ImeMode = System::Windows::Forms::ImeMode::NoControl;
            this->cbVar_usl->Location = System::Drawing::Point(153, 84);
            this->cbVar_usl->Name = L"cbVar_usl";
            this->cbVar_usl->Size = System::Drawing::Size(155, 21);
            this->cbVar_usl->TabIndex = 54;
            // 
            // cbStat_usl
            // 
            this->cbStat_usl->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbStat_usl->FormattingEnabled = true;
            this->cbStat_usl->Location = System::Drawing::Point(77, 61);
            this->cbStat_usl->Name = L"cbStat_usl";
            this->cbStat_usl->Size = System::Drawing::Size(231, 21);
            this->cbStat_usl->TabIndex = 53;
            // 
            // cbPerk_usl
            // 
            this->cbPerk_usl->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbPerk_usl->FormattingEnabled = true;
            this->cbPerk_usl->Location = System::Drawing::Point(77, 38);
            this->cbPerk_usl->Name = L"cbPerk_usl";
            this->cbPerk_usl->Size = System::Drawing::Size(231, 21);
            this->cbPerk_usl->TabIndex = 52;
            // 
            // cbSkill_usl
            // 
            this->cbSkill_usl->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbSkill_usl->FormattingEnabled = true;
            this->cbSkill_usl->Location = System::Drawing::Point(77, 16);
            this->cbSkill_usl->Name = L"cbSkill_usl";
            this->cbSkill_usl->Size = System::Drawing::Size(231, 21);
            this->cbSkill_usl->TabIndex = 51;
            // 
            // rbVar
            // 
            this->rbVar->AutoSize = true;
            this->rbVar->Location = System::Drawing::Point(5, 85);
            this->rbVar->Name = L"rbVar";
            this->rbVar->Size = System::Drawing::Size(63, 17);
            this->rbVar->TabIndex = 50;
            this->rbVar->Text = L"Перем.";
            this->rbVar->UseVisualStyleBackColor = true;
            // 
            // rbStat
            // 
            this->rbStat->AutoSize = true;
            this->rbStat->Location = System::Drawing::Point(5, 62);
            this->rbStat->Name = L"rbStat";
            this->rbStat->Size = System::Drawing::Size(45, 17);
            this->rbStat->TabIndex = 49;
            this->rbStat->Text = L"Stat";
            this->rbStat->UseVisualStyleBackColor = true;
            // 
            // rbPerk
            // 
            this->rbPerk->AutoSize = true;
            this->rbPerk->Location = System::Drawing::Point(5, 39);
            this->rbPerk->Name = L"rbPerk";
            this->rbPerk->Size = System::Drawing::Size(48, 17);
            this->rbPerk->TabIndex = 48;
            this->rbPerk->Text = L"Perk";
            this->rbPerk->UseVisualStyleBackColor = true;
            // 
            // rbSkill
            // 
            this->rbSkill->AutoSize = true;
            this->rbSkill->Checked = true;
            this->rbSkill->Location = System::Drawing::Point(5, 16);
            this->rbSkill->Name = L"rbSkill";
            this->rbSkill->Size = System::Drawing::Size(45, 17);
            this->rbSkill->TabIndex = 47;
            this->rbSkill->TabStop = true;
            this->rbSkill->Text = L"Skill";
            this->rbSkill->UseVisualStyleBackColor = true;
            // 
            // btnUslRez
            // 
            this->btnUslRez->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 14, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
                static_cast<System::Byte>(204)));
            this->btnUslRez->ForeColor = System::Drawing::Color::Red;
            this->btnUslRez->Location = System::Drawing::Point(314, 16);
            this->btnUslRez->Name = L"btnUslRez";
            this->btnUslRez->Size = System::Drawing::Size(32, 89);
            this->btnUslRez->TabIndex = 46;
            this->btnUslRez->Text = L">";
            this->btnUslRez->UseVisualStyleBackColor = true;
            // 
            // label28
            // 
            this->label28->AutoSize = true;
            this->label28->Location = System::Drawing::Point(153, 156);
            this->label28->Name = L"label28";
            this->label28->Size = System::Drawing::Size(30, 13);
            this->label28->TabIndex = 45;
            this->label28->Text = L"мин.";
            // 
            // numLock
            // 
            this->numLock->Location = System::Drawing::Point(77, 154);
            this->numLock->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {9999, 0, 0, 0});
            this->numLock->Name = L"numLock";
            this->numLock->Size = System::Drawing::Size(70, 20);
            this->numLock->TabIndex = 44;
            // 
            // rbLock
            // 
            this->rbLock->AutoSize = true;
            this->rbLock->Location = System::Drawing::Point(5, 154);
            this->rbLock->Name = L"rbLock";
            this->rbLock->Size = System::Drawing::Size(69, 17);
            this->rbLock->TabIndex = 43;
            this->rbLock->Text = L"Блок. на";
            this->rbLock->UseVisualStyleBackColor = true;
            // 
            // cbQuest_usl
            // 
            this->cbQuest_usl->Location = System::Drawing::Point(0, 0);
            this->cbQuest_usl->Name = L"cbQuest_usl";
            this->cbQuest_usl->Size = System::Drawing::Size(121, 21);
            this->cbQuest_usl->TabIndex = 0;
            // 
            // label21
            // 
            this->label21->AutoSize = true;
            this->label21->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.25F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
                static_cast<System::Byte>(204)));
            this->label21->Location = System::Drawing::Point(311, 162);
            this->label21->Name = L"label21";
            this->label21->Size = System::Drawing::Size(35, 13);
            this->label21->TabIndex = 22;
            this->label21->Text = L"если";
            // 
            // cbQuest_rez
            // 
            this->cbQuest_rez->Location = System::Drawing::Point(0, 0);
            this->cbQuest_rez->Name = L"cbQuest_rez";
            this->cbQuest_rez->Size = System::Drawing::Size(121, 21);
            this->cbQuest_rez->TabIndex = 0;
            // 
            // numUslRezValue
            // 
            this->numUslRezValue->Location = System::Drawing::Point(352, 53);
            this->numUslRezValue->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {-2147483640, 0, 0, 0});
            this->numUslRezValue->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) {-2147483640, 0, 0, 0});
            this->numUslRezValue->Name = L"numUslRezValue";
            this->numUslRezValue->Size = System::Drawing::Size(215, 20);
            this->numUslRezValue->TabIndex = 19;
            this->numUslRezValue->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {-2147483640, 0, 0, 0});
            // 
            // cbItem_usl
            // 
            this->cbItem_usl->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbItem_usl->FormattingEnabled = true;
            this->cbItem_usl->Location = System::Drawing::Point(153, 108);
            this->cbItem_usl->Name = L"cbItem_usl";
            this->cbItem_usl->Size = System::Drawing::Size(155, 21);
            this->cbItem_usl->TabIndex = 5;
            // 
            // numItem
            // 
            this->numItem->Location = System::Drawing::Point(77, 108);
            this->numItem->Name = L"numItem";
            this->numItem->Size = System::Drawing::Size(70, 20);
            this->numItem->TabIndex = 4;
            // 
            // rbQuest
            // 
            this->rbQuest->AutoSize = true;
            this->rbQuest->Location = System::Drawing::Point(5, 131);
            this->rbQuest->Name = L"rbQuest";
            this->rbQuest->Size = System::Drawing::Size(56, 17);
            this->rbQuest->TabIndex = 3;
            this->rbQuest->Text = L"Квест";
            this->rbQuest->UseVisualStyleBackColor = true;
            // 
            // cbPlayerType
            // 
            this->cbPlayerType->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
            this->cbPlayerType->FormattingEnabled = true;
            this->cbPlayerType->Items->AddRange(gcnew cli::array< System::Object^  >(3) {L"Для текущего игрока", L"Для всех игроков", 
                L"Для всех кроме текущего"});
            this->cbPlayerType->Location = System::Drawing::Point(153, 132);
            this->cbPlayerType->Name = L"cbPlayerType";
            this->cbPlayerType->Size = System::Drawing::Size(155, 21);
            this->cbPlayerType->TabIndex = 2;
            // 
            // rbItem
            // 
            this->rbItem->AutoSize = true;
            this->rbItem->Location = System::Drawing::Point(5, 109);
            this->rbItem->Name = L"rbItem";
            this->rbItem->Size = System::Drawing::Size(53, 17);
            this->rbItem->TabIndex = 0;
            this->rbItem->Text = L"Итем";
            this->rbItem->UseVisualStyleBackColor = true;
            // 
            // numQuest
            // 
            this->numQuest->Location = System::Drawing::Point(77, 131);
            this->numQuest->Name = L"numQuest";
            this->numQuest->Size = System::Drawing::Size(70, 20);
            this->numQuest->TabIndex = 1;
            // 
            // Form1
            // 
            this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->ClientSize = System::Drawing::Size(692, 470);
            this->Controls->Add(this->statusStrip1);
            this->Controls->Add(this->tabControl1);
            //this->Icon = (cli::safe_cast<System::Drawing::Icon^  >(resources->GetObject(L"$this.Icon")));
            this->Location = System::Drawing::Point(9, 40);
            this->MinimumSize = System::Drawing::Size(700, 460);
            this->Name = L"Form1";
            this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
            this->Text = L"===";
            this->FormClosed += gcnew System::Windows::Forms::FormClosedEventHandler(this, &Form1::Form1_FormClosed);
            this->Load += gcnew System::EventHandler(this, &Form1::Form1_Load);
            this->statusStrip1->ResumeLayout(false);
            this->statusStrip1->PerformLayout();
            this->tabControl1->ResumeLayout(false);
            this->tabDialogs->ResumeLayout(false);
            this->tabControl2->ResumeLayout(false);
            this->tabDialogsMenu->ResumeLayout(false);
            this->tabDialogsMenu->PerformLayout();
            this->gbDialogsTexts->ResumeLayout(false);
            this->gbDialogsTexts->PerformLayout();
            this->gbDialogsDB->ResumeLayout(false);
            this->gbDialogsDB->PerformLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numDialogsId))->EndInit();
            this->tabInfo->ResumeLayout(false);
            this->tabInfo->PerformLayout();
            this->groupBox1->ResumeLayout(false);
            this->groupBox1->PerformLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numInfoNum))->EndInit();
            this->tabDialog->ResumeLayout(false);
            this->tabDialog->PerformLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numIDdialog))->EndInit();
            this->tabAnswer->ResumeLayout(false);
            this->tabAnswer->PerformLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numLink))->EndInit();
            this->tabDemand->ResumeLayout(false);
            this->tabDemand->PerformLayout();
            this->gbDemandScript->ResumeLayout(false);
            this->gbDemandScript->PerformLayout();
            this->panel2->ResumeLayout(false);
            this->panel2->PerformLayout();
            this->tabResult->ResumeLayout(false);
            this->tabResult->PerformLayout();
            this->gbResultScript->ResumeLayout(false);
            this->gbResultScript->PerformLayout();
            this->panel1->ResumeLayout(false);
            this->panel1->PerformLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numResultLock))->EndInit();
            this->tabText->ResumeLayout(false);
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->gridText))->EndInit();
            this->tabComment->ResumeLayout(false);
            this->tabVars->ResumeLayout(false);
            this->groupBox2->ResumeLayout(false);
            this->groupBox8->ResumeLayout(false);
            this->groupBox8->PerformLayout();
            this->groupBox3->ResumeLayout(false);
            this->groupBox3->PerformLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numVarMax))->EndInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numVarId))->EndInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numVarMin))->EndInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numVarStart))->EndInit();
            this->tbMain->ResumeLayout(false);
            this->tbMain->PerformLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numLock))->EndInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numUslRezValue))->EndInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numItem))->EndInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->numQuest))->EndInit();
            this->ResumeLayout(false);
            this->PerformLayout();

        }
#pragma endregion

#pragma region Editor

#define LANGUAGE_RUSSIAN		0
#define LANGUAGE_ENGLISH		1

private: System::Void rbRussian_CheckedChanged(System::Object^  sender, System::EventArgs^  e)
{
	if(rbRussian->Checked) Lang=LANGUAGE_RUSSIAN;
	else if(rbEnglish->Checked) Lang=LANGUAGE_ENGLISH;
	Localize();
}

private: System::Void textBox1_TextChanged(System::Object^  sender, System::EventArgs^  e)
{
	Localize();
}

String^ LocalizeText(String^ rus, String^ eng)
{
	switch(Lang)
	{
	case LANGUAGE_RUSSIAN: return rus;
	case LANGUAGE_ENGLISH: return eng;
	default: return rus;
	}
}

void Localize()
{
	this->lSSLabel1->Text = LocalizeText("Последняя ошибка:","Last error:");
	this->tabInfo->Text = LocalizeText("Инфо","Info");
	this->tabText->Text = LocalizeText("Строки","Strings");
	this->tabComment->Text = LocalizeText("Комментарий","Comment");
	this->tabDialogs->Text = LocalizeText("Редактор диалогов","Dialogs editor");
	this->btnDeleteLine->Text = LocalizeText("Удалить строку","Delete node");
	this->tabDialogsMenu->Text = LocalizeText("Опции","Options");
	this->label24->Text = LocalizeText("Текущий диалог:","Current dialog:");
	this->btnDlgSave->Text = LocalizeText("Сохранить","Save");
	this->gbDialogsTexts->Text = LocalizeText("Тексты","Texts");
	this->btnTextCheck->Text = LocalizeText("Удалить битые ссылки","Delete bad links");
	this->btnDialogAddTree->Text = LocalizeText("Добавить дерево","Add tree");
	this->btnTreeToMain->Text = LocalizeText("Сделать дерево основным","Set tree to main");
	this->btnReloadTexts->Text = LocalizeText("Перегрузить тексты","Reload texts");
	this->label42->Text = LocalizeText("Язык","Lang");
	this->btnDlgSaveAs->Text = LocalizeText("Сохранить как","Save As");
	this->btnDlgLoad->Text = LocalizeText("Загрузить","Load");
	this->gbDialogsDB->Text = LocalizeText("База данных","Data base");
	this->btnDialogErase->Text = LocalizeText("Удалить","Delete");
	this->label36->Text = LocalizeText("Номер","Number");
	this->label35->Text = LocalizeText("Имя файла","File name");
	this->btnDialogSave->Text = LocalizeText("Сохранить","Save");
	this->label30->Text = LocalizeText("Неизвестные переменные:","Unknown vars:");
	this->label27->Text = LocalizeText("Число кривых ответов: ","Count bad links: ");
	this->btnDialogCheck->Text = LocalizeText("Проверить диалог","Check dialog");
	this->btnDialogClear->Text = LocalizeText("Очистить","Clear");
	this->tabDialog->Text = LocalizeText("Диалог","Dialog");
	this->label40->Text = LocalizeText("Текст диалога","Dialog text");
	this->label26->Text = LocalizeText("Script","Script");
	this->cbAID->Text = LocalizeText("Авто Id","Auto Id");
	this->label1->Text = LocalizeText("Номер диалога","Dialog number");
	this->btnAddDialog->Text = LocalizeText("Добавить диалог","Add dialog");
	this->txtDialog->Text = LocalizeText("Текст диалога","Dialog text");
	this->cbDialogNoShuffle->Text = LocalizeText("Не перемешивать ответы","No shuffle answers");
	this->tabAnswer->Text = LocalizeText("Ответ","Answer");
	this->cbLinkOther->Items[0]=LocalizeText("Закрыть диалог","Close dialog");
	this->cbLinkOther->Items[1]=LocalizeText("На предыдущий диалог","On previous dialog");
	this->cbLinkOther->Items[2]=LocalizeText("Бартер","Barter");
	this->cbLinkOther->Items[3]=LocalizeText("Атаковать","Attack");
	this->rbLinkOther->Text = LocalizeText("Другой","Other");
	this->rbLinkId->Text = LocalizeText("К диалогу","On dialog");
	this->label39->Text = LocalizeText("Текст ответа","Answer text");
	this->txtAnswer->Text = LocalizeText("Текст ответа","Answer text");
	this->btnAddAnswer->Text = LocalizeText("Добавить ответ","Add answer");
	this->tabDemand->Text = LocalizeText("Условие","Demand");
	this->rbDemandScript->Text = LocalizeText("Скрипт (модуль@функция)","Script (module@func)");
	this->rbDemandScriptVal0->Text = LocalizeText("Нет","NA");
	this->gbDemandScript->Text = LocalizeText("Аргументы","Arguments");
	this->label63->Text = LocalizeText("Условие для","Demand for");
	this->rbDemandNpc->Text = LocalizeText("Нпц","Npc");
	this->rbDemandPlayer->Text = LocalizeText("Игрока","Player");
	this->btnDemandAdd->Text = LocalizeText("Добавить","Add");
	this->btnDemandOr->Text = LocalizeText("Или","Or");
	this->label19->Text = LocalizeText("Значение условия","Demand value");
	this->rbDemandVar->Text = LocalizeText("Перем.","Var");
	this->rbDemandParam->Text = LocalizeText("Параметр","Parameter");
	this->rbDemandItem->Text = LocalizeText("Итем","Item");
	this->cbDemandNoRecheck->Text = LocalizeText("Не перепроверять","No recheck");
	this->tabResult->Text = LocalizeText("Результат","Result");
	this->rbResultScript->Text = LocalizeText("Скрипт (модуль@функция)","Script (module@func)");
	this->gbResultScript->Text = LocalizeText("Аргументы","Arguments");
	this->rbResultScriptVal0->Text = LocalizeText("Нет","NA");
	this->label62->Text = LocalizeText("Результат для","Result for");
	this->rbResultNpc->Text = LocalizeText("Нпц","Npc");
	this->rbResultPlayer->Text = LocalizeText("Игрока","Player");
	this->btnResultAdd->Text = LocalizeText("Добавить","Add");
	this->label41->Text = LocalizeText("мин.","min.");
	this->radioButton8->Text = LocalizeText("Блокировка ответа на","Block answer on");
	this->label43->Text = LocalizeText("Значение результата","Result value");
	this->rbResultVar->Text = LocalizeText("Перем.","Var");
	this->rbResultParam->Text = LocalizeText("Параметр","Parameter");
	this->rbResultItem->Text = LocalizeText("Итем","Item");
	this->tabVars->Text = LocalizeText("Редактор переменных","Vars editor");
	this->rbVarUnicum->Text = LocalizeText("Уникальная","Unicum");
	this->rbVarLocal->Text = LocalizeText("Локальная","Local");
	this->rbVarGlobal->Text = LocalizeText("Глобальная","Global");
	this->rbVarLocalLocation->Text = LocalizeText("Локальная локации","Local location");
	this->rbVarLocalMap->Text = LocalizeText("Локальная карты","Local map");
	this->rbVarLocalItem->Text = LocalizeText("Локальная предмета","Local item");
	this->btnVarDelete->Text = LocalizeText("Удалить (по номеру)","Delete (by number)");
	this->btnVarAdd->Text = LocalizeText("Добавить","Add");
	this->groupBox2->Text = LocalizeText("Списки переменных","Vars lists");
	this->groupBox8->Text = LocalizeText("Флаги","");
	this->cbVarRandom->Text = LocalizeText("Рандомная","Random");
	this->cbVarQuest->Text = LocalizeText("Квестовая","Quest");
	this->cbVarNoCheck->Text = LocalizeText("Без границ","No borders");
	this->groupBox3->Text = LocalizeText("Описание параметра","Parameter info");
	this->btnVarChange->Text = LocalizeText("Изменить","Change");
	this->label20->Text = LocalizeText("Макс. значение","Max value");
	this->label23->Text = LocalizeText("Номер","Num");
	this->label25->Text = LocalizeText("Мин. значение","Min value");
	this->label37->Text = LocalizeText("Имя и описание","Name and desc");
	this->label38->Text = LocalizeText("Стартовое зн.","Start value");
	this->radioButton1->Text = LocalizeText("Перки","Perks");
	this->btnSkill->Text = LocalizeText("Скилы","Skills");
	this->btnStat->Text = LocalizeText("Статы","Stats");
	this->localBtn->Text = LocalizeText("Локал. переменная","Local var");
	this->playerBtn->Text = LocalizeText("Переменная игрока","Player var");
	this->globalBtn->Text = LocalizeText("Глоб. переменная","Global var");
	this->label31->Text = LocalizeText("Макс. значение","Max value");
	this->label29->Text = LocalizeText("Номер параметра","Parameter number");
	this->label33->Text = LocalizeText("Мин. значение","Min value");
	this->lbIntro->Text = LocalizeText("Имя параметра","Parameter name");
	this->label34->Text = LocalizeText("Значение","Value");
	this->btnUslRezDelete->Text = LocalizeText("Удалить","Delete");
	this->rbRez->Text = LocalizeText("Результат","Result");
	this->rbUsl->Text = LocalizeText("Условие","Demand");
	this->btnUslRezChange->Text = LocalizeText("Изменить","Edit");
	this->btnUslRezAdd->Text = LocalizeText("Добавить","Add");
	this->label22->Text = LocalizeText("Значение условия/результата","Value of demand/result");
	this->rbVar->Text = LocalizeText("Перем.","Var");
	this->rbStat->Text = LocalizeText("Стат","Stat");
	this->rbPerk->Text = LocalizeText("Перк","Perk");
	this->rbSkill->Text = LocalizeText("Скилл","Skill");
	this->label28->Text = LocalizeText("мин.","min.");
	this->rbLock->Text = LocalizeText("Блок. на","Block on");
	this->cbAction_no_answ->Items[0]=LocalizeText("None","None");
	this->cbAction_no_answ->Items[1]=LocalizeText("Attack","Attack");
	this->lInfoName->Text=LocalizeText("Имя","Name");
	this->lInfoAvatar->Text=LocalizeText("Аватар","Avatar");
	this->groupBox1->Text=LocalizeText("Описание","Description");
	this->rbInfoLive->Text=LocalizeText("Живой","Alive");
	this->rbInfoKnockout->Text=LocalizeText("В накауте","Knockout");
	this->rbInfoDead->Text=LocalizeText("Мертвый","Dead");
	this->rbInfoCriticalDead->Text=LocalizeText("Жестокая смерть","Critical dead");
	this->lInfoNum->Text=LocalizeText("Номер","Number");
	this->lInfoFull->Text=LocalizeText("Полное","Full");
	this->lInfoShort->Text=LocalizeText("Краткое","Short");
	this->enD.Text=LocalizeText("Редактировать диалог","Edit dialog");
	this->enD.cbDialogNoShuffle->Text=LocalizeText("Не перемешивать ответы","No shuffle answers");
	this->enD.Text=LocalizeText("Редактировать диалог","Edit dialog");
	this->enD.btnOk->Text=LocalizeText("Изменить","Change");
	this->enD.btnCancel->Text=LocalizeText("Отмена","Cancel");
	this->enA.Text=LocalizeText("Редактировать ответ","Edit answer");
	this->enA.rbLinkId->Text=LocalizeText("К диалогу","On dialog");
	this->enA.rbLinkOther->Text=LocalizeText("Другой","Other");
	this->enA.cbLinkOther->Items[0]=LocalizeText("Закрыть диалог","Close dialog");
	this->enA.cbLinkOther->Items[1]=LocalizeText("На предыдущий диалог","On previous dialog");
	this->enA.cbLinkOther->Items[2]=LocalizeText("Бартер","Barter");
	this->enA.cbLinkOther->Items[3]=LocalizeText("Атаковать","Attack");
	this->enA.btnOk->Text=LocalizeText("Изменить","Change");
	this->enA.btnCancel->Text=LocalizeText("Отмена","Cancel");

	this->lInfoAbout->Text=LocalizeText("Краткое описание выводится без клика по Нпц, полное при клике.\n"
		"Если краткое или полное описание не найдено, то описание составляется с использованием имени.\n"
		"Например: Вы видите: Незнакомец. Вы видите труп: Незнакомец. и т.д.\n"
		"Если и имя не найдено, то выдается шаблонная фраза что Вы не видите ничего необычного.",
		"Short description is screens without a click on NPC, expanded - when clicked.\n"
		"If short and large descriptions are not found then description compounds with the usage of the name.\n"
		"Example: You see: Stranger. You see a body: Stranger. and so on.\n"
		"If the name is not found then screened a pattern phrase that you seen nothing unusual.");
}

private: System::Void tabControl1_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e){}

private: System::Void Form1_Load(System::Object^  sender, System::EventArgs^  e)
{
	this->Text="Dialog editor   "+DIALOG_EDITOR_VERSION_STR;
	Environment::CurrentDirectory=AppDomain::CurrentDomain->SetupInformation->ApplicationBase;

	srand(GetTickCount());
	tabControl1->SelectedIndex=0;
	setlocale(LC_ALL,"Russian");

	char names[1024];
	GetPrivateProfileString("Options","Names",".\\data\\",names,1024,".\\DialogEditor.cfg");
	ConstantsManager::Initialize(-1,names);

	ParamsName=gcnew cli::array<String^>(MAX_PARAMS);
	ParamsDesc=gcnew cli::array<String^>(MAX_PARAMS);
	LangTree=gcnew cli::array<TreeView^>(0);

	array<String^>^ arg=Environment::GetCommandLineArgs();
	Environment::CurrentDirectory=arg[0]->Substring(0,arg[0]->LastIndexOf("\\"));
	String^ fname="";
	for(int i=1;i<arg->Length;i++)
	{
		if(arg[i]->Contains(DIALOG_FILE_EXT)) fname=arg[i];
	}

	Lang=GetPrivateProfileInt("Options","Language",LANGUAGE_RUSSIAN,".\\DialogEditor.cfg");
	String^ cmd=Environment::CommandLine;
	if(cmd->Contains("-ru")) Lang=LANGUAGE_RUSSIAN;
	if(cmd->Contains("-en")) Lang=LANGUAGE_ENGLISH;

	LogToFile("DialogEditor.log");

	Localize();
	if(Lang==LANGUAGE_RUSSIAN) rbRussian->Checked=true;
	else if(Lang==LANGUAGE_ENGLISH) rbEnglish->Checked=true;
	ResetAll();

	char vars[1024];
	GetPrivateProfileString("Options","Vars",".\\scripts\\",vars,1024,".\\DialogEditor.cfg");
	if(!VarMngr.Init(vars)) Close();
	UpdateAll();

	if(fname->Length && DlgLoad(fname)) lDlgName->Text=fname;

//////////////////////////////////////////////////////////////////////////
/*	FILE* file=fopen(".\\dlg_from\\dialogs.lst","rb");
	if(!file) return;

	DWORD dlg_num;
	char dlg_name[128];
	char ch=0;

	while(!feof(file))
	{
		fscanf(file,"%c",&ch);
		if(ch!='$') continue;

		if(fscanf(file,"%u%s",&dlg_num,dlg_name)!=2) continue;

		String^ name=ToClrString(dlg_name);
		if(DlgLoadOld(".\\dlg_from\\"+name+".dlg",dlg_num)) DlgSave(".\\dlg_to\\"+name+".fodlg");
	}

	fclose(file);*/
}

private: System::Void Form1_FormClosed(System::Object^  sender, System::Windows::Forms::FormClosedEventArgs^  e)
{
	VarMngr.Finish();
	DlgMngr.Finish();
}

private: System::Void ResetAll()
{
	ResetNpc();
	ResetDialog();
	ResetVar();
}

void UpdateAll()
{
	UpdateListBoxes();
	UpdateVarsList();
	UpdateDlgList();
	UpdateDialog();
}

private: System::Void btnDialogAddTree_Click(System::Object^  sender, System::EventArgs^  e)
{
	EntBox.ShowDialog();
	if(!EntBox.IsOk) return;
	String^ name=EntBox.Text;
	if(name->Length!=4) lErr->Text=LocalizeText("Необходимо 4 символа.","Need 4 chars.");
	else
	{
		if(IsNpcInfo(name))
		{
			lErr->Text="Tree already exist.";
			return;
		}
		TreeView^ tree=CreateNewEditBox(name);
		NpcInfo info;
		info.PackName=ToAnsi(name);
		info.Name="none";
		AllNpcInfo.push_back(info);
		if(LangTree->Length==1) AddDialog(tree,1,"None",true,LocalizeText("Преддиалоговые установки","Predialogue installations"));
		else
		{
			array<TreeNode^>^ arr=gcnew array<TreeNode^>(LangTree[0]->Nodes->Count);
			LangTree[0]->Nodes->CopyTo(arr,0);
			for(int i=0;i<LangTree[0]->Nodes->Count;i++) tree->Nodes->Add((TreeNode^)arr[i]->Clone());
		}
	}
}

private: System::Void btnTreeToMain_Click(System::Object^  sender, System::EventArgs^  e)
{
	if(tcLang->SelectedIndex<1) return;
	int index=tcLang->SelectedIndex;
	TreeView^ first_tree=GetMainTree();
	NpcInfo& first_info=GetNpcInfo(first_tree->Name);
	TreeView^ cur_tree=GetCurTree();
	NpcInfo& cur_info=GetNpcInfo(cur_tree->Name);

	// Swap tabs
	TabPage^ first_tab=tcLang->TabPages[0];
	TabPage^ cur_tab=tcLang->TabPages[index];
	first_tab->Controls->Clear();
	first_tab->Controls->Add(first_tree);
	cur_tab->Controls->Clear();
	cur_tab->Controls->Add(btnUpLine);
	cur_tab->Controls->Add(btnDownLine);
	cur_tab->Controls->Add(btnDeleteLine);
	cur_tab->Controls->Add(cur_tree);
	tcLang->TabPages->RemoveAt(index);
	tcLang->TabPages->Insert(0,cur_tab);

	// Swap trees
	LangTree[0]=cur_tree;
	LangTree[index]=first_tree;

	// Swap info
	NpcInfo tmp_info=first_info;
	first_info=cur_info;
	cur_info=tmp_info;

	UpdateDialog();
	lErr->Text="Ok.";
}

System::Windows::Forms::TreeView^ CreateNewEditBox(String^ name)
{
	System::Windows::Forms::TreeView^ tree=gcnew System::Windows::Forms::TreeView();
	tree = (gcnew System::Windows::Forms::TreeView());
	tree->BackColor = System::Drawing::Color::White;
	tree->Dock = System::Windows::Forms::DockStyle::Fill;
	tree->Font = (gcnew System::Drawing::Font(L"Comic Sans MS", 8.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
		static_cast<System::Byte>(204)));
	tree->ForeColor = System::Drawing::SystemColors::Window;
	tree->HideSelection = false;
	tree->HotTracking = true;
	tree->LineColor = System::Drawing::Color::Red;
	tree->Location = System::Drawing::Point(3, 3);
	tree->Name = name;
	tree->ShowNodeToolTips = true;
	tree->Size = System::Drawing::Size(670, 175);
	tree->MouseClick += gcnew System::Windows::Forms::MouseEventHandler(this, &Form1::Tree_MouseClick);
	tree->AfterSelect += gcnew System::Windows::Forms::TreeViewEventHandler(this, &Form1::Tree_AfterSelect);
	tree->NodeMouseClick += gcnew System::Windows::Forms::TreeNodeMouseClickEventHandler(this, &Form1::Tree_NodeMouseClick);
	tree->MouseCaptureChanged += gcnew System::EventHandler(this, &Form1::Tree_MouseCaptureChanged);
	tree->Click += gcnew System::EventHandler(this, &Form1::Tree_Click);

	System::Windows::Forms::TabPage^ tab=(gcnew System::Windows::Forms::TabPage());
	tab->SuspendLayout();
	if(LangTree->Length==0)
	{
		tab->Controls->Add(btnUpLine);
		tab->Controls->Add(btnDownLine);
		tab->Controls->Add(btnDeleteLine);
		btnUpLine->Location = System::Drawing::Point(497, 229);
		btnDeleteLine->Location = System::Drawing::Point(447, 262);
		btnDownLine->Location = System::Drawing::Point(497, 297);
	}
	tab->Controls->Add(tree);
	tab->Location=System::Drawing::Point(4, 22);
	tab->Name="tb"+name;
	tab->Padding=System::Windows::Forms::Padding(3);
	tab->Size=System::Drawing::Size(585, 346);
	tab->Text=name;
	tab->UseVisualStyleBackColor=true;
	tab->ResumeLayout(false);
	tcLang->Controls->Add(tab);

	Array::Resize(LangTree,LangTree->Length+1);
	LangTree[LangTree->Length-1]=tree;
	return tree;
}

void UpdateDialogText()
{
	TreeView^ tree=GetCurTree();
	if(!tree) return;
	NpcInfo& info=GetNpcInfo(tree->Name);

	gridText->Rows->Clear();
	for(UIntStrMulMap::iterator it=info.Texts.begin();it!=info.Texts.end();it++)
	{
		if(!(*it).second.length()) continue;
		int row=gridText->Rows->Add();
		gridText->Rows[row]->Cells[0]->Value=Convert::ToString((int)(*it).first);
		gridText->Rows[row]->Cells[1]->Value=ToClrString((*it).second.c_str());
	}
}

void SaveDialogText()
{
	TreeView^ tree=GetCurTree();
	if(!tree) return;
	NpcInfo& info=GetNpcInfo(tree->Name);

	info.Texts.clear();
	for(int i=0;i<gridText->RowCount;i++)
	{
		DWORD num=Convert::ToInt32(gridText->Rows[i]->Cells[0]->Value);
		string str=ToAnsi(Convert::ToString(gridText->Rows[i]->Cells[1]->Value));
		info.Texts.insert(UIntStrMulMap::value_type(num,str));
	}

	/*if(info.Texts.empty()) return;
	for(int i=0,j=AllNpcInfo.size();i<j;i++)
	{
		NpcInfo& info2=AllNpcInfo[i];
		if(&info==&info2) continue;
		for(UIntStrMulMap::iterator it=info.Texts.end();;)
		{
			it--;
			DWORD num=(*it).first;
			if(info2.Texts.count(num)<info.Texts.count(num) && (*it).second.length()) info2.Texts.insert(UIntStrMulMap::value_type(num,(*it).second));
			if(it==info.Texts.begin()) break;
		}
	}*/
}

private: System::Void gridText_CellEndEdit(System::Object^  sender, System::Windows::Forms::DataGridViewCellEventArgs^  e)
{
	SaveDialogText();
}

private: System::Void gridText_KeyPress(System::Object^  sender, System::Windows::Forms::KeyPressEventArgs^  e){}
private: System::Void gridText_KeyUp(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e)
{
	if(e->KeyCode==Keys::F5) UpdateDialogText();
	if(e->KeyCode==Keys::F6)
	{
		int row=gridText->CurrentCell->RowIndex;
		DWORD num=Convert::ToInt32(gridText->Rows[row]->Cells[0]->Value);
		string val=ToAnsi(Convert::ToString(gridText->Rows[row]->Cells[1]->Value));
		if(!val.length()) return;
		TreeView^ tree=GetCurTree();
		if(!tree) return;
		NpcInfo& info=GetNpcInfo(tree->Name);
		for(int i=0,j=AllNpcInfo.size();i<j;i++)
		{
			NpcInfo& info_=AllNpcInfo[i];
			if(&info==&info_) continue;
			info_.Texts.insert(UIntStrMulMap::value_type(num,val));
		}
	}
}
private: System::Void gridText_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e)
{
	//lErr->Text="Down";
}

static bool DisableSaveNpcInfo=false;
void UpdateNpcInfo()
{
	TreeView^ tree=GetCurTree();
	if(!tree) return;
	NpcInfo& info=GetNpcInfo(tree->Name);

	DisableSaveNpcInfo=true;

	tInfoName->Text=ToClrString(info.Name);
	tInfoAvatar->Text=ToClrString(info.Avatar);
	int ind=0;
	if(rbInfoKnockout->Checked) ind=2;
	if(rbInfoDead->Checked) ind=4;
	if(rbInfoCriticalDead->Checked) ind=6;

	int num=int(numInfoNum->Value-1);
	StrVec& v=info.Info[ind];
	if(num<(int)v.size()) rtInfoShort->Text=ToClrString(v[num]);
	else rtInfoShort->Text="";

	StrVec& v_=info.Info[ind+1];
	if(num<(int)v_.size()) rtInfoFull->Text=ToClrString(v_[num]);
	else rtInfoFull->Text="";

	DisableSaveNpcInfo=false;
}

void SaveNpcInfo()
{
	if(DisableSaveNpcInfo) return;
	TreeView^ tree=GetCurTree();
	if(!tree) return;
	NpcInfo& info=GetNpcInfo(tree->Name);

	info.Name=ToAnsi(tInfoName->Text);
	info.Avatar=ToAnsi(tInfoAvatar->Text);
	int ind=0;
	if(rbInfoKnockout->Checked) ind=2;
	if(rbInfoDead->Checked) ind=4;
	if(rbInfoCriticalDead->Checked) ind=6;

	int num=int(numInfoNum->Value-1);
	StrVec& v=info.Info[ind];
	if(num>=(int)v.size()) v.resize(num+1);
	v[num]=ToAnsi(rtInfoShort->Text);

	StrVec& v_=info.Info[ind+1];
	if(num>=(int)v_.size()) v_.resize(num+1);
	v_[num]=ToAnsi(rtInfoFull->Text);
}

bool IsNpcInfo(String^ lang)
{
	for(int i=0,j=AllNpcInfo.size();i<j;i++)
	{
		NpcInfo& info=AllNpcInfo[i];
		if(ToClrString(info.PackName)==lang) return true;
	}
	return false;
}

NpcInfo& GetNpcInfo(String^ lang)
{
	for(int i=0,j=AllNpcInfo.size();i<j;i++)
	{
		NpcInfo& info=AllNpcInfo[i];
		if(ToClrString(info.PackName)==lang) return info;
	}
	NpcInfo info_;
	info_.PackName=ToAnsi(lang);
	info_.Name="error_";
	info_.Avatar="";
	AllNpcInfo.push_back(info_);
	return AllNpcInfo[AllNpcInfo.size()-1];
}

private: System::Void tInfoName_TextChanged(System::Object^  sender, System::EventArgs^  e)
{
	SaveNpcInfo();
}

private: System::Void tcLang_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e)
{
	UpdateNpcInfo();
	UpdateDialogText();
}

private: System::Void rbInfoLive_CheckedChanged(System::Object^  sender, System::EventArgs^  e)
{
	UpdateNpcInfo();
}

private: System::Void numInfoNum_ValueChanged(System::Object^  sender, System::EventArgs^  e)
{
	UpdateNpcInfo();
}

String^ GetParamDesc(String^ str)
{
	for(int i=0;i<ParamsName->Length;++i)
		if(ParamsName[i]==str) return ParamsName[i];
	return str;
}

System::Void UpdateListBoxes()
{
	// Items
	cbDemandItem->Items->Clear();
	cbResultItem->Items->Clear();
	StrVec item_names=ConstantsManager::GetCollection(CONSTANTS_ITEM);
	for(StrVec::iterator it=item_names.begin(),end=item_names.end();it!=end;++it)
	{
		cbDemandItem->Items->Add(ToClrString(*it));
		cbResultItem->Items->Add(ToClrString(*it));
	}
 
	// Parameters
	cbDemandParam->Items->Clear();
	cbResultParam->Items->Clear();
	StrVec param_names=ConstantsManager::GetCollection(CONSTANTS_PARAM);
	int i=0;
	for(StrVec::iterator it=param_names.begin(),end=param_names.end();it!=end;++it)
	{
		ParamsName[i]=ToClrString(*it);
		ParamsDesc[i]=ToClrString(*it);
		cbDemandParam->Items->Add(ToClrString(*it));
		cbResultParam->Items->Add(ToClrString(*it));
		i++;
	}

	// Defines
	cbDemandValue->Items->Clear();
	cbResultValue->Items->Clear();
	cbDemandSValue0->Items->Clear();
	cbDemandSValue1->Items->Clear();
	cbDemandSValue2->Items->Clear();
	cbDemandSValue3->Items->Clear();
	cbDemandSValue4->Items->Clear();
	cbResultSValue0->Items->Clear();
	cbResultSValue1->Items->Clear();
	cbResultSValue2->Items->Clear();
	cbResultSValue3->Items->Clear();
	cbResultSValue4->Items->Clear();
	StrVec define_names=ConstantsManager::GetCollection(CONSTANTS_DEFINE);
	for(StrVec::iterator it=define_names.begin(),end=define_names.end();it!=end;++it)
	{
		cbDemandValue->Items->Add(ToClrString(*it));
		cbResultValue->Items->Add(ToClrString(*it));
		cbDemandSValue0->Items->Add(ToClrString(*it));
		cbDemandSValue1->Items->Add(ToClrString(*it));
		cbDemandSValue2->Items->Add(ToClrString(*it));
		cbDemandSValue3->Items->Add(ToClrString(*it));
		cbDemandSValue4->Items->Add(ToClrString(*it));
		cbResultSValue0->Items->Add(ToClrString(*it));
		cbResultSValue1->Items->Add(ToClrString(*it));
		cbResultSValue2->Items->Add(ToClrString(*it));
		cbResultSValue3->Items->Add(ToClrString(*it));
		cbResultSValue4->Items->Add(ToClrString(*it));
	}
}

#pragma endregion

#pragma region Npc Edit

void ResetNpc()
{

}	

#pragma endregion

#pragma region Dialogs Edit

TreeView^ GetMainTree()
{
	if(tcLang->SelectedIndex<0) return nullptr;
	return LangTree[0];
}

TreeView^ GetCurTree()
{
	if(tcLang->SelectedIndex<0) return nullptr;
	return LangTree[tcLang->SelectedIndex];
}

TreeNode^ GetTreeNode(TreeView^ tree, TreeNode^ src_node)
{
	int level=src_node->Level;
	IntVec lvls;

	for(int i=0;i<=level;i++)
	{
		lvls.push_back(src_node->Index);
		src_node=src_node->Parent;
	}

	TreeNode^ res=tree->Nodes[(lvls[lvls.size()-1])];
	for(int i=(int)lvls.size()-2;i>=0;i--)
	{
		res=res->Nodes[lvls[i]];
	}

	return res;
}

private: System::Void btnDialogSave_Click(System::Object^  sender, System::EventArgs^  e){}
private: System::Void btnDialogErase_Click(System::Object^  sender, System::EventArgs^  e){}

void ResetDialog()
{
	// Dialog menu
	btnDialogClear->Enabled=true;
	btnDialogCheck->Enabled=false;
	btnDialogSave->Enabled=false;

	btnDeleteLine->Enabled=false;
	btnUpLine->Enabled=false;
	btnDownLine->Enabled=false;

	txtDialogName->Text="";
	numDialogsId->Value=0;

	lbDialogsList->Items->Clear();

	// Dialog
	txtDialog->Text=LocalizeText("Текст диалога","Dialog text");
	numIDdialog->Value=2;
	cbAction_no_answ->SelectedIndex=0;
	cbAID->Checked=true;
	btnAddDialog->Enabled=true;
	DialogOptions.clear();

	// Answer
	txtAnswer->Text=LocalizeText("Текст ответа","Answer text");
	numLink->Value=numLink->Minimum;
	btnAddAnswer->Enabled=true;

	// Demand
	rbDemandParam->Checked=true;
	cbDemandParam->Items->Clear();

	btnDemandOper->Text=">";
	cbDemandValue->Text="";

	// Result
	rbResultParam->Checked=true;
	cbResultParam->Items->Clear();

	btnResultOper->Text="+";
	cbResultValue->Text="";
	numResultLock->Value=1000000;

	// Erase tabs
	tcLang->Controls->Clear();
	LangTree=gcnew cli::array<TreeView^>(0);
	AllNpcInfo.clear();
	rtComment->Text="";
	tInfoName->Text="";
	tInfoAvatar->Text="";
	rtInfoFull->Text="";
	rtInfoShort->Text="";
	numInfoNum->Value=1;
	rbInfoLive->Checked=true;
}

void UpdateDialog()
{
	btnDialogClear->Enabled=true;
	btnDialogCheck->Enabled=true;
	btnDialogSave->Enabled=true;
	btnDeleteLine->Enabled=true;
	btnUpLine->Enabled=true;
	btnDownLine->Enabled=true;

	UpdateDlgList();
	UpdateListBoxes();
	UpdateNpcInfo();
	UpdateDialogText();
}

private: System::Void btnDialogClear_Click(System::Object^  sender, System::EventArgs^  e)
{
	if(MessageBox::Show("Really?","NPC Editor",
		MessageBoxButtons::YesNo,
		MessageBoxIcon::Information)==System::Windows::Forms::DialogResult::No) return;

	ResetDialog();
	UpdateDialog();

	lDlgName->Text="";
}

private: System::Void cbDemandOper_Click(System::Object^  sender, System::EventArgs^  e)
{
	if(btnDemandOper->Text == ">") btnDemandOper->Text = "<";
	else if(btnDemandOper->Text == "<") btnDemandOper->Text = "==";
	else if(btnDemandOper->Text == "==") btnDemandOper->Text = "!=";
	else if(btnDemandOper->Text == "!=") btnDemandOper->Text = ">=";
	else if(btnDemandOper->Text == ">=") btnDemandOper->Text = "<=";
	else if(btnDemandOper->Text == "<=") btnDemandOper->Text = ">";
}

private: System::Void btnResultOper_Click(System::Object^  sender, System::EventArgs^  e)
{
	if(btnResultOper->Text == "+") btnResultOper->Text = "-";
	else if(btnResultOper->Text == "-") btnResultOper->Text = "*";
	else if(btnResultOper->Text == "*") btnResultOper->Text = "/";
	else if(btnResultOper->Text == "/") btnResultOper->Text = "=";
	else if(btnResultOper->Text == "=") btnResultOper->Text = "+";
}

private: System::Void tabNpc_Click(System::Object^  sender, System::EventArgs^  e)
{
}

private: System::Void btnUpLine_Click(System::Object^  sender, System::EventArgs^  e)
{
	MoveNode(-1);
}
private: System::Void btnDownLine_Click(System::Object^  sender, System::EventArgs^  e)
{
	MoveNode(1);
}

private: System::Void MoveNode(int count)
{
	if(!GetMainTree()->SelectedNode)
	{
		lErr->Text=LocalizeText("Выберите ветку диалога или ответа.","Choose dialog or answer node.");
		return;
	}

	TreeNode^ dlg_r;
	int ins_ind=0;

	if(GetMainTree()->SelectedNode->Level==0)
	{
		if(GetMainTree()->SelectedNode->Index==0)
		{
			lErr->Text=LocalizeText("Этот диалог нельзя перемещать.","This dialog is not removable.");
			return;
		}

		dlg_r=GetMainTree()->SelectedNode;
		ins_ind=GetMainTree()->SelectedNode->Index;
		if(ins_ind+count==0)
		{
			lErr->Text=LocalizeText("Нельзя переместить выше.","Can't move upper.");
			return;
		}

		// Move ext
		for(int i=1,j=LangTree->Length;i<j;i++)
		{
			TreeView^ tree=LangTree[i];
			TreeNode^ dlg_e=GetTreeNode(tree,dlg_r);
			dlg_e->Remove();
			tree->Nodes->Insert(ins_ind+count,dlg_e);
			tree->SelectedNode=dlg_e;
		}

		// Move main
		dlg_r->Remove();
		GetMainTree()->Nodes->Insert(ins_ind+count,dlg_r);
		GetMainTree()->SelectedNode=dlg_r;
	}
	else
	{
		dlg_r=GetMainTree()->SelectedNode;

		// Move ext
		for(int i=1,j=LangTree->Length;i<j;i++)
		{
			TreeView^ tree=LangTree[i];
			TreeNode^ dlg_e=GetTreeNode(tree,dlg_r);
			tree->SelectedNode=dlg_e;
			TreeNode^ dlg2=tree->SelectedNode->Parent;
			ins_ind=tree->SelectedNode->Index;
			dlg_e->Remove();
			dlg2->Nodes->Insert(ins_ind+count,dlg_e);
			tree->SelectedNode=dlg_e;
		}

		// Move main
		TreeNode^ dlg2=GetMainTree()->SelectedNode->Parent;
		ins_ind=GetMainTree()->SelectedNode->Index;
		dlg_r->Remove();
		dlg2->Nodes->Insert(ins_ind+count,dlg_r);
		GetMainTree()->SelectedNode=dlg_r;
	}
}

private: System::Void btnDeleteLine_Click(System::Object^  sender, System::EventArgs^  e)
{
	TreeNode ^ dlg_r=GetMainTree()->SelectedNode;

	if(GetMainTree()->GetNodeCount(false) && dlg_r)
	{
		if(dlg_r->Level==0 && dlg_r->Index==0)
		{
			lErr->Text=LocalizeText("Этот диалог нельзя удалить.","This dialog is not removable.");
			return;
		}

		// Delete ext
		for(int i=1,j=LangTree->Length;i<j;i++)
		{
			TreeView^ tree=LangTree[i];
			TreeNode^ dlg_e=GetTreeNode(tree,dlg_r);

			if(dlg_e->Level==3 && dlg_e->Parent->Nodes->Count<=1)
				dlg_e=dlg_e->Parent;
			tree->Nodes->Remove(dlg_e);
		}

		//GetMainTree()
		if(dlg_r->Level==3 && dlg_r->Parent->Nodes->Count<=1)
			dlg_r=dlg_r->Parent;
		GetMainTree()->Nodes->Remove(dlg_r);
	}
	else
	{
		lErr->Text=LocalizeText("Нет ни одного диалога.","No one dialog.");
		return;
	}
}

private: System::Void Tree_MouseClick(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e){}
private: System::Void Tree_MouseCaptureChanged(System::Object^  sender, System::EventArgs^  e){}
private: System::Void Tree_Click(System::Object^  sender, System::EventArgs^  e){}
private: System::Void Tree_AfterSelect(System::Object^  sender, System::Windows::Forms::TreeViewEventArgs^  e){}

private: System::Void Tree_NodeMouseClick(System::Object^  sender, System::Windows::Forms::TreeNodeMouseClickEventArgs^  e)
{
	TreeView^ tree=dynamic_cast<TreeView^>(sender);
	if(!tree) return;
	tree->SelectedNode=e->Node;
	if(tree->SelectedNode->Level<2 && e->Button==System::Windows::Forms::MouseButtons::Right) EditNode(tree,tree!=GetMainTree());
}

private: System::Void EditNode(TreeView^ tree, bool only_text)
{
	TreeNode ^ dlg = tree->SelectedNode;

	if(dlg->Level==0)
	{
		if(dlg->Name=="1")
		{
			lErr->Text=LocalizeText("Этот диалог нельзя редактировать.","Can't edit this dialog.");
			return;
		}

		enD.num->Text=dlg->Name;

		INT32 pos=tree->SelectedNode->Text->IndexOf("]");
		enD.text->Text=tree->SelectedNode->Text->Substring(pos+2);

		enD.cbScript->SelectedIndex=-1;
		enD.cbScript->Text=ToClrString(DialogOptions[int(enD.num->Value)].DlgScript);
		enD.cbDialogNoShuffle->Checked=DialogOptions[int(enD.num->Value)].DlgNoShuffle;
		enD.ShowDialog();

		if(enD.edit==true)
		{
			if(only_text==false)
			{
				if(tree->Nodes->IndexOfKey(enD.num->Text)!=-1 && enD.num->Text!=dlg->Name)
				{
					lErr->Text=LocalizeText("Диалог под таким номером уже создан.","Dialog with this number is already created.");
				}
				else
				{
					// Если был изменен номер диалога, то нужна изменить номера
					// всех ответов, ссылающихся на этот диалог.
					cli::array<TreeNode^>^ edit_tree;
					edit_tree=tree->Nodes->Find(dlg->Name,true);

					for(int et=0;et<edit_tree->Length;et++)
					{
						if(edit_tree[et]->Level==1)
						{
							edit_tree[et]->Name=enD.num->Text;
							edit_tree[et]->Text="[->"+edit_tree[et]->Name+"] "+edit_tree[et]->ToolTipText;

						//Edit ext
							for(int i=1,j=LangTree->Length;i<j;i++)
							{
								TreeView^ tree=LangTree[i];
								TreeNode^ dlg_e=GetTreeNode(tree,edit_tree[et]);
								dlg_e->Name=enD.num->Text;
								dlg_e->Text="[->"+dlg_e->Name+"] "+dlg_e->ToolTipText;
							}
						}
					}
				}

				DialogOptions[int(enD.num->Value)].DlgScript=ToAnsi(enD.cbScript->Text);
				DialogOptions[int(enD.num->Value)].DlgNoShuffle=enD.cbDialogNoShuffle->Checked;
				dlg->Name=enD.num->Text;
				dlg->ToolTipText="Script: "+enD.cbScript->Text;

				// Edit ext
				for(int i=1,j=LangTree->Length;i<j;i++)
				{
					TreeView^ tree=LangTree[i];
					TreeNode^ dlg_e=GetTreeNode(tree,dlg);
					dlg_e->Name=enD.num->Text;
					dlg_e->ToolTipText="Script: "+enD.cbScript->Text;
					dlg_e->Text="["+dlg_e->Name+"] "+dlg_e->Text->Substring(dlg_e->Text->IndexOf(']')+2);
				}
			}

			dlg->Text="["+dlg->Name+"] " + enD.text->Text;
		}
	}
	else if(tree->SelectedNode->Level==1)
	{
		enA.answer->Text = dlg->ToolTipText;

		enA.rbLinkOther->Checked=true;
		switch(Convert::ToInt32(dlg->Name))
		{
		case DIALOG_END: enA.cbLinkOther->SelectedIndex=0; break;
		case DIALOG_BACK: enA.cbLinkOther->SelectedIndex=1; break;
		case DIALOG_BARTER: enA.cbLinkOther->SelectedIndex=2; break;
		case DIALOG_ATTACK: enA.cbLinkOther->SelectedIndex=3; break;
		default:
			enA.rbLinkId->Checked=true;
			enA.link->Text=dlg->Name;
			break;
		}

		enA.ShowDialog();

again:
		if(enA.edit==true) //GetMainTree()
		{
			if(only_text==false)
			{
				UINT link=0;
				char link_name[128];

				if(enA.rbLinkId->Checked==true)
				{
					if(tree->Nodes->IndexOfKey(enA.link->Text) == -1)
					{
						lErr->Text=LocalizeText("Нет диалога, на который ссылается ответ.","Dialog with this link not found.");
						enA.ShowDialog();
						goto again;
					}
					link=(UINT)enA.link->Value;
					sprintf(link_name,"%d",link);
				}
				else if(enA.rbLinkOther->Checked==true)
				{
					switch(enA.cbLinkOther->SelectedIndex)
					{
					case 0: link=DIALOG_END; strcpy(link_name,ToAnsi(LocalizeText("Выход","Exit"))); break;
					case 1: link=DIALOG_BACK; strcpy(link_name,ToAnsi(LocalizeText("Назад","Back"))); break;
					case 2: link=DIALOG_BARTER; strcpy(link_name,ToAnsi(LocalizeText("Бартер","Barter"))); break;
					case 3: link=DIALOG_ATTACK; strcpy(link_name,ToAnsi(LocalizeText("Атака","Attack"))); break;
					default:
						lErr->Text="Unknown link.";
						enA.ShowDialog();
						goto again;
					}
				}

				dlg->Name = Convert::ToString(link);
				dlg->ToolTipText=enA.answer->Text;
				dlg->Text = "[->"+ToClrString(link_name)+"] " + dlg->ToolTipText;

				// Edit ext
				for(int i=1,j=LangTree->Length;i<j;i++)
				{
					TreeView^ tree=LangTree[i];
					TreeNode^ dlg_e=GetTreeNode(tree,dlg);
					dlg_e->Name=dlg->Name;
					dlg_e->Text = "[->"+ToClrString(link_name)+"] " + dlg_e->ToolTipText;
				}
			}

			dlg->ToolTipText=enA.answer->Text;
			dlg->Text=dlg->Text->Insert(dlg->Text->IndexOf(']')+2,dlg->ToolTipText+"\0");
			dlg->ForeColor=dlg->ForeColor.Green;
			lErr->Text="---";
		}
	}
}

void UpdateDlgList()
{
	lbDialogsList->Items->Clear();
	for(StrUIntMap::iterator it=DlgMngr.DlgPacksNames.begin();it!=DlgMngr.DlgPacksNames.end();++it)
	{
		lbDialogsList->Items->Add(((Int32)(*it).second).ToString()+" - "+ToClrString((*it).first.c_str()));
	}
}

private: System::Void lbDialogsList_MouseDoubleClick(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e){}
#pragma endregion

#pragma region Rebuild Dialog GetMainTree()
void LoadTree(String^ lang, DialogsVec& dlg_pack, FOMsg& texts)
{
	// Info
	NpcInfo info;
	info.PackName=ToAnsi(lang);
	info.Name=texts.GetStr(100);
	info.Avatar=(texts.Count(110)?texts.GetStr(110):"");
	for(int i=0;i<8;i++)
		for(int l=0,k=texts.Count(200+i*10);l<k;l++)
			info.Info[i].push_back(texts.GetStr(200+i*10,l));

	// Texts
	UIntStrMulMap& txt=texts.GetData();
	UIntStrMulMap::iterator it_txt=txt.upper_bound(99999999);
	while(it_txt!=txt.end())
	{
		info.Texts.insert(UIntStrMulMap::value_type((*it_txt).first-100000000,(*it_txt).second));
		it_txt++;
	}

	AllNpcInfo.push_back(info);

	// Tree
	TreeView^ tree=CreateNewEditBox(lang);
	tree->Nodes->Clear();
	for(DialogsVec::iterator it=dlg_pack.begin();it!=dlg_pack.end();++it)
	{
		Dialog& dlg=*it;

		TreeNode^ dlg_node=AddDialog(tree,dlg.Id,dlg.DlgScript.c_str(),dlg.IsNoShuffle(),ToClrString(texts.GetStr(dlg.TextId)));
		if(!dlg_node)
		{
			lErr->Text="RebuildDlgTree - Error AddDialog <"+((Int32)dlg.Id).ToString()+">";
			continue;
		}

		for(AnswersVec::iterator it_a=dlg.Answers.begin(),it_a_end=dlg.Answers.end();it_a!=it_a_end;++it_a)
		{
			DialogAnswer& answ=*it_a;
			TreeNode^ answ_node=AddAnswer(dlg_node,answ.Link,ToClrString(texts.GetStr(answ.TextId)));
			if(!answ_node)
			{
				lErr->Text="RebuildDlgTree - Error AddAnswer <"+((Int32)answ.Link).ToString()+">";
				continue;
			}

			for(DemandResultVec::iterator it_d(answ.Demands.begin()),it_d_end(answ.Demands.end());it_d!=it_d_end;++it_d)
			{
				DemandResult& d=*it_d;

				TreeNode ^ demand_node;
				if(answ_node->Nodes->IndexOfKey("demand") == -1)
				{
					demand_node = answ_node->Nodes->Add("demand",LocalizeText("Условие","Demand"));
					demand_node->ForeColor = demand_node->BackColor.Black;
				}
				else demand_node = answ_node->Nodes->Find("demand",false)[0];

				if(!AddDemand(demand_node,d))
				{
					lErr->Text="RebuildDlgTree - Error AddDemand type<"+((Int32)d.Type).ToString()+">, name<"+ToClrString(d.ParamName.c_str())+">";
					continue;
				}
			}

			for(DemandResultVec::iterator it_r(answ.Results.begin()),it_r_end(answ.Results.end());it_r!=it_r_end;++it_r)
			{
				DemandResult& r=*it_r;
				TreeNode ^ result_node;
				if(answ_node->Nodes->IndexOfKey("result") == -1)
				{
					result_node = answ_node->Nodes->Add("result",LocalizeText("Результат","Result"));
					result_node->ForeColor = result_node->BackColor.Black;
				}
				else result_node = answ_node->Nodes->Find("result",false)[0];

				if(!AddResult(result_node,r))
				{
					lErr->Text="RebuildDlgTree - Error AddResult type<"+((Int32)r.Type).ToString()+">, name<"+ToClrString(r.ParamName.c_str())+">";
					continue;
				}
			}
		}
	}
}

#pragma endregion

#pragma region Dialog Add Dialog
private: System::Void btnAddDialog_Click(System::Object^  sender, System::EventArgs^  e)
{
	if(GetMainTree()->Nodes->IndexOfKey(numIDdialog->Text)!=-1)
	{
		lErr->Text=LocalizeText("Диалог под таким номером уже создан.","Dialog with this number is already exist.");
		return;
	}

	// Add all
	for(int i=0,j=LangTree->Length;i<j;i++)
	{
		TreeView^ tree=LangTree[i];
		AddDialog(tree,int(numIDdialog->Value),ToAnsi(cbAction_no_answ->Text),cbDialogNoShuffle->Checked,txtDialog->Text);
	}

	numLink->Value = numIDdialog->Value;

	if(cbAID->Checked==true)
	{
		for (;;)
		{
			numIDdialog->Value++;
			if(GetMainTree()->Nodes->IndexOfKey(numIDdialog->Text)==-1) break;
			if(numIDdialog->Value>=numIDdialog->Maximum) break;
		}
	}
}

private: TreeNode ^ AddDialog(TreeView^ tree, int num, const char* na, bool no_shuffle, String ^ text)
{
	if(tree->Nodes->IndexOfKey(num.ToString()) != -1) 
	{ 
		lErr->Text=LocalizeText("Диалог под таким номером уже создан.","Dialog with this number is already exist.");
		return nullptr;
	}

	if(text == nullptr) return nullptr; 

	TreeNode ^ dlg = tree->Nodes->Add(num.ToString(), "[" + num.ToString() + "] " + text);

	if(num==1)
	{
		dlg->ForeColor=dlg->BackColor.BlueViolet;
		dlg->ToolTipText=LocalizeText("Преддиалоговые установки","Predialogue installations");
	}
	else
	{
		dlg->ForeColor=dlg->BackColor.Blue;
		dlg->ToolTipText="Script: " + ToClrString(na);
	}

	DialogOptions[num].DlgScript=na;
	DialogOptions[num].DlgNoShuffle=no_shuffle;
	return dlg;
}
#pragma endregion

#pragma region Dialog Add Answer
private: System::Void btnAddAnswer_Click(System::Object^  sender, System::EventArgs^  e)
{
	if(!GetMainTree()->SelectedNode || GetMainTree()->SelectedNode->Level!=0)
	{
		lErr->Text=LocalizeText("Выберите ноду диалога.","Choose dialog node.");
		return;
	}

	WORD link;

	if(rbLinkId->Checked==true)
	{
		if(GetMainTree()->Nodes->IndexOfKey(numLink->Text) != -1)
		{
			//	AddAnswer(Tree->SelectedNode,int(numLink->Value),-1,txtAnswer->Text);
			link=(int)numLink->Value;
		}
		else
		{
			lErr->Text=LocalizeText("Нет диалога, на который ссылается ответ.","Dialog with this link not found.");
			return;
		}
	}
	else if(rbLinkOther->Checked==true)
	{
		switch(cbLinkOther->SelectedIndex)
		{
		case 0: link=DIALOG_END; break;
		case 1: link=DIALOG_BACK; break;
		case 2: link=DIALOG_BARTER; break;
		case 3: link=DIALOG_ATTACK; break;
		default:
			lErr->Text="Unknown link.";
			return;
		}
	}

	// Add main
	TreeNode^ cur_r=GetMainTree()->SelectedNode;
	AddAnswer(cur_r,link,txtAnswer->Text);

	// Add ext
	for(int i=1,j=LangTree->Length;i<j;i++)
	{
		TreeView^ tree=LangTree[i];
		TreeNode^ cur_e=GetTreeNode(tree,cur_r);
		AddAnswer(cur_e,link,txtAnswer->Text);
	}
}

private: TreeNode ^ AddAnswer(TreeNode ^ dlg, int num, System::String ^ str)
{
	if(!dlg)
	{
		lErr->Text=LocalizeText("Для данного ответа нет диалога.","Dialog for this answer not found.");
		return nullptr;
	}

	TreeNode^ answ;
	answ=dlg->Nodes->Add(num.ToString(),"null");
	answ->ToolTipText = str;

	char link_name[128];

	switch(num)
	{
	case DIALOG_END: strcpy(link_name,ToAnsi(LocalizeText("Выход","Exit"))); break;
	case DIALOG_BACK: strcpy(link_name,ToAnsi(LocalizeText("Назад","Back"))); break;
	case DIALOG_BARTER: strcpy(link_name,ToAnsi(LocalizeText("Бартер","Barter"))); break;
	case DIALOG_ATTACK: strcpy(link_name,ToAnsi(LocalizeText("Атака","Attack"))); break;
	default: sprintf(link_name,"%d",num); break;
	}

	answ->Text="[->"+ToClrString(link_name)+"] "+answ->ToolTipText;
	answ->ForeColor=answ->BackColor.Green;
	return answ;
}
#pragma endregion

#pragma region Dialog Add Demand
private: System::Void btnDemandAdd_Click(System::Object^  sender, System::EventArgs^  e)
{
	if(!GetMainTree()->SelectedNode || GetMainTree()->SelectedNode->Level!=1)
	{
		lErr->Text=LocalizeText("Выберите ответ для условия.","Choose answer for this demand.");
		return;
	}

	DemandResult demand;

	if(btnDemandOper->Text==">") demand.Op='>';
	else if(btnDemandOper->Text=="<") demand.Op='<';
	else if(btnDemandOper->Text=="==") demand.Op='=';
	else if(btnDemandOper->Text=="!=") demand.Op='!';
	else if(btnDemandOper->Text==">=") demand.Op='}';
	else if(btnDemandOper->Text=="<=") demand.Op='{';
	else return;

	if(rbDemandPlayer->Checked)
		demand.Who='p';
	else
		demand.Who='n';

	demand.ValueStr=ToAnsi(cbDemandValue->Text);
	demand.NoRecheck=(bool)cbDemandNoRecheck->Checked;
   
	if(sender==btnDemandOr)
	{
		demand.Type=DR_OR;
	}
	else if(rbDemandParam->Checked)
	{
		if(cbDemandParam->SelectedIndex==-1) return;
		demand.Type=DR_PARAM;
		demand.ParamName=ToAnsi(ParamsName[cbDemandParam->SelectedIndex]);
		int id=ConstantsManager::GetParamId(demand.ParamName.c_str());
		if(id==-1) return;
		demand.ParamId=id;
	}
	else if(rbDemandVar->Checked)
	{
		if(cbDemandVars->SelectedIndex==-1)
		{
			lErr->Text=LocalizeText("Выберите имя переменной.","Choose var name.");
			return;
		}

		demand.Type=DR_VAR;
		demand.ParamName=string(ToAnsi(cbDemandVars->Text));
	}
	else if(rbDemandItem->Checked)
	{
		if(cbDemandItem->SelectedIndex==-1) return;
		demand.Type=DR_ITEM;
		int pid=ConstantsManager::GetItemPid(ToAnsi(cbDemandItem->Text));
		if(pid==-1) return;
		demand.ParamId=pid;
	}
	else if(rbDemandScript->Checked)
	{
		if(txtDemandScript->Text->Length<=3)
		{
			lErr->Text=LocalizeText("Слишком короткое имя.","Name is too short.");
			return;
		}

		demand.Type=DR_SCRIPT;
		demand.ParamName=string(ToAnsi(txtDemandScript->Text));
		if(rbDemandScriptVal0->Checked) demand.ValuesCount=0;
		else if(rbDemandScriptVal1->Checked) demand.ValuesCount=1;
		else if(rbDemandScriptVal2->Checked) demand.ValuesCount=2;
		else if(rbDemandScriptVal3->Checked) demand.ValuesCount=3;
		else if(rbDemandScriptVal4->Checked) demand.ValuesCount=4;
		else if(rbDemandScriptVal5->Checked) demand.ValuesCount=5;
		demand.ValuesNames[0]=ToAnsi(cbDemandSValue0->Text);
		demand.ValuesNames[1]=ToAnsi(cbDemandSValue1->Text);
		demand.ValuesNames[2]=ToAnsi(cbDemandSValue2->Text);
		demand.ValuesNames[3]=ToAnsi(cbDemandSValue3->Text);
		demand.ValuesNames[4]=ToAnsi(cbDemandSValue4->Text);
	}
	else return;

	TreeNode^ cur_r=GetMainTree()->SelectedNode;

	if(cur_r->Nodes->IndexOfKey("demand") == -1)
	{
		// Add ext
		for(int i=1,j=LangTree->Length;i<j;i++)
		{
			TreeView^ tree=LangTree[i];
			TreeNode^ cur_e=GetTreeNode(tree,cur_r);
			TreeNode^ demand_node_e=cur_e->Nodes->Add("demand",LocalizeText("Условие","Demand"));
			demand_node_e->ForeColor=demand_node_e->BackColor.Black;
			AddDemand(demand_node_e,demand);
		}

		// Add main
		TreeNode^ demand_node_r=cur_r->Nodes->Add("demand",LocalizeText("Условие","Demand"));
		demand_node_r->ForeColor=demand_node_r->BackColor.Black;
		AddDemand(demand_node_r,demand);
	}
	else
	{
		// Add ext
		for(int i=1,j=LangTree->Length;i<j;i++)
		{
			TreeView^ tree=LangTree[i];
			TreeNode^ cur_e=GetTreeNode(tree,cur_r);
			TreeNode^ demand_node_e=cur_e->Nodes->Find("demand",false)[0];
			AddDemand(demand_node_e,demand);
		}

		// Add main
		TreeNode^ demand_node_r=cur_r->Nodes->Find("demand",false)[0];
		AddDemand(demand_node_r,demand);
	}
}


private: TreeNode^ AddDemand(TreeNode^ u_node, DemandResult& d)
{
	if(!u_node) return nullptr;
	if(d.Type==DR_NONE || d.Type==DR_LOCK) return nullptr;

	char name[1024];
	char text[1024];

	char who[128];
	if(d.Who=='p') strcpy(who,ToAnsi(LocalizeText("Игрок:","Player:")));
	else if(d.Who=='n') strcpy(who,ToAnsi(LocalizeText("Нпц:","Npc:")));
	else return nullptr;

	if(d.Type==DR_PARAM)
	{
		sprintf(name,"_param %c %s %c %s",d.Who,d.ParamName.c_str(),d.Op,d.ValueStr.c_str());
		sprintf(text,"%s %s %s %s %s",who,ToAnsi(LocalizeText("Параметр","Parameter")),GetParamDesc(ToClrString(d.ParamName.c_str())),GetOpName(true,d.Op).c_str(),d.ValueStr.c_str());
	}
	else if(d.Type==DR_VAR)
	{
		sprintf(name,"_var %c %s %c %s",d.Who,d.ParamName.c_str(),d.Op,d.ValueStr.c_str());
		sprintf(text,"%s %s %s %s %s",who,ToAnsi(LocalizeText("Переменная","Var")),d.ParamName.c_str(),GetOpName(true,d.Op).c_str(),d.ValueStr.c_str());
	}
	else if(d.Type==DR_ITEM) 
	{
		const char* s=ConstantsManager::GetItemName(d.ParamId);
		char buf[64];
		sprintf(name,"_item %c %s %c %s",d.Who,s?s:itoa(d.ParamId,buf,10),d.Op,d.ValueStr.c_str());
		sprintf(text,"%s %s %s %s %s",who,ToAnsi(LocalizeText("Итем","Item")),s?s:itoa(d.ParamId,buf,10),GetOpName(true,d.Op).c_str(),d.ValueStr.c_str());
	}
	else if(d.Type==DR_SCRIPT) 
	{
		char values[256]={0};
		if(d.ValuesCount==1) sprintf(values,"%s",d.ValuesNames[0].c_str());
		else if(d.ValuesCount==2) sprintf(values,"%s %s",d.ValuesNames[0].c_str(),d.ValuesNames[1].c_str());
		else if(d.ValuesCount==3) sprintf(values,"%s %s %s",d.ValuesNames[0].c_str(),d.ValuesNames[1].c_str(),d.ValuesNames[2].c_str());
		else if(d.ValuesCount==4) sprintf(values,"%s %s %s %s",d.ValuesNames[0].c_str(),d.ValuesNames[1].c_str(),d.ValuesNames[2].c_str(),d.ValuesNames[3].c_str());
		else if(d.ValuesCount==5) sprintf(values,"%s %s %s %s %s",d.ValuesNames[0].c_str(),d.ValuesNames[1].c_str(),d.ValuesNames[2].c_str(),d.ValuesNames[3].c_str(),d.ValuesNames[4].c_str());
		sprintf(name,"_script %s %d ",d.ParamName.c_str(),d.ValuesCount);
		strcat(name,values);
		sprintf(text,"%s %s (",ToAnsi(LocalizeText("Скрипт","Script")),d.ParamName.c_str());
		strcat(text,values);
		strcat(text,")");
	}
	else if(d.Type==DR_OR)
	{
		sprintf(name,"or");
		sprintf(text,"%s",ToAnsi(LocalizeText("Или","Or")));

	}
	else
		return nullptr;

	if(d.NoRecheck)
	{
		char buf[1024];
		strcpy(buf,name);
		strcpy(name,"no_recheck ");
		strcat(name,buf);
		strcat(text,ToAnsi(LocalizeText(" Не перепроверять"," No recheck")));
	}

	TreeNode^ d_node=u_node->Nodes->Add(ToClrString(name),ToClrString(text));
	d_node->ForeColor=System::Drawing::Color::Purple;
	return d_node;
}
#pragma endregion

#pragma region Dialog Add Result
private: System::Void btnResultAdd_Click(System::Object^  sender, System::EventArgs^  e)
{
	if(!GetMainTree()->SelectedNode || GetMainTree()->SelectedNode->Level!=1)
	{
		lErr->Text=LocalizeText("Выберите ответ для результата.","Choose answer for result.");
		return;
	}

	DemandResult result;

	if(btnResultOper->Text=="+") result.Op='+';
	else if(btnResultOper->Text=="-") result.Op='-';
	else if(btnResultOper->Text=="*") result.Op='*';
	else if(btnResultOper->Text=="/") result.Op='/';
	else if(btnResultOper->Text=="=") result.Op='=';
	else return;

	if(rbResultPlayer->Checked==true)
		result.Who='p';
	else
		result.Who='n';

	result.ValueStr=ToAnsi(cbResultValue->Text);

	if(rbResultParam->Checked)
	{
		if(cbResultParam->SelectedIndex==-1) return;
		result.Type=DR_PARAM;
		result.ParamName=ToAnsi(ParamsName[cbResultParam->SelectedIndex]);
		int id=ConstantsManager::GetParamId(result.ParamName.c_str());
		if(id==-1) return;
		result.ParamId=id;
	}
	else if(rbResultVar->Checked)
	{
		if(cbResultVars->SelectedIndex==-1)
		{
			lErr->Text=LocalizeText("Выберите имя переменной.","Choose var name.");
			return;
		}

		result.Type=DR_VAR;
		result.ParamName=string(ToAnsi(cbResultVars->Text));
	}
	else if(rbResultItem->Checked)
	{
		if(cbResultItem->SelectedIndex==-1) return;
		result.Type=DR_ITEM;
		int pid=ConstantsManager::GetItemPid(ToAnsi(cbResultItem->Text));
		if(pid==-1) return;
		result.ParamId=pid;
	}
	else if(rbResultScript->Checked)
	{
		if(rbResultScript->Text->Length<=3)
		{
			lErr->Text=LocalizeText("Слишком короткое имя.","Name too short.");
			return;
		}

		result.Type=DR_SCRIPT;
		result.ParamName=string(ToAnsi(txtResultScript->Text));
		if(rbResultScriptVal0->Checked) result.ValuesCount=0;
		else if(rbResultScriptVal1->Checked) result.ValuesCount=1;
		else if(rbResultScriptVal2->Checked) result.ValuesCount=2;
		else if(rbResultScriptVal3->Checked) result.ValuesCount=3;
		else if(rbResultScriptVal4->Checked) result.ValuesCount=4;
		else if(rbResultScriptVal5->Checked) result.ValuesCount=5;
		result.ValuesNames[0]=ToAnsi(cbResultSValue0->Text);
		result.ValuesNames[1]=ToAnsi(cbResultSValue1->Text);
		result.ValuesNames[2]=ToAnsi(cbResultSValue2->Text);
		result.ValuesNames[3]=ToAnsi(cbResultSValue3->Text);
		result.ValuesNames[4]=ToAnsi(cbResultSValue4->Text);
	}
	else return;

	TreeNode^ cur_r=GetMainTree()->SelectedNode;

	if(cur_r->Nodes->IndexOfKey("result") == -1)
	{
		//Add ext
		for(int i=1,j=LangTree->Length;i<j;i++)
		{
			TreeView^ tree=LangTree[i];
			TreeNode^ cur_e=GetTreeNode(tree,cur_r);
			TreeNode^ demand_node_e=cur_e->Nodes->Add("result",LocalizeText("Результат","Result"));
			demand_node_e->ForeColor=demand_node_e->BackColor.Black;
			AddResult(demand_node_e,result);
		}

		//Add main
		TreeNode^ demand_node_r=cur_r->Nodes->Add("result",LocalizeText("Результат","Result"));
		demand_node_r->ForeColor=demand_node_r->BackColor.Black;
		AddResult(demand_node_r,result);
	}
	else
	{
		// Add ext
		for(int i=1,j=LangTree->Length;i<j;i++)
		{
			TreeView^ tree=LangTree[i];
			TreeNode^ cur_e=GetTreeNode(tree,cur_r);
			TreeNode^ demand_node_e=cur_e->Nodes->Find("result",false)[0];
			AddResult(demand_node_e,result);
		}

		// Add main
		TreeNode^ demand_node_r=cur_r->Nodes->Find("result",false)[0];
		AddResult(demand_node_r,result);
	}
}

private: TreeNode ^ AddResult(TreeNode^ r_node, DemandResult& r)
{
	if(!r_node) return nullptr;
	if(r.Type==DR_NONE) return nullptr;

	String^ name;
	String^ text;
	char buffer[1024];

	char who[128];
	if(r.Who=='p') strcpy(who,ToAnsi(LocalizeText("Игрок:","Player:")));
	else if(r.Who=='n') strcpy(who,ToAnsi(LocalizeText("Нпц:","Npc:")));
	else return nullptr;

	if(r.Type==DR_PARAM)
	{
		sprintf(buffer,"_param %c %s %c %s",r.Who,r.ParamName.c_str(),r.Op,r.ValueStr.c_str());
		name=ToClrString(buffer);
		sprintf(buffer, "%s %s %s %s %s",who,ToAnsi(LocalizeText("Параметр","Parameter")),GetParamDesc(ToClrString(r.ParamName.c_str())),GetOpName(false,r.Op).c_str(),r.ValueStr.c_str());
		text=ToClrString(buffer);
	}
	else if(r.Type==DR_VAR)
	{
		sprintf(buffer,"_var %c %s %c %s",r.Who,r.ParamName.c_str(),r.Op,r.ValueStr.c_str());
		name=ToClrString(buffer);
		sprintf(buffer,"%s %s %s %s %s",who,ToAnsi(LocalizeText("Переменная","Var")),r.ParamName.c_str(),GetOpName(false,r.Op).c_str(),r.ValueStr.c_str());
		text=ToClrString(buffer);
	}
	else if(r.Type==DR_ITEM) 
	{
		const char* s=ConstantsManager::GetItemName(r.ParamId);
		char buf[64];
		sprintf(buffer,"_item %c %s %c %s",r.Who,s?s:itoa(r.ParamId,buf,10),r.Op,r.ValueStr.c_str());
		name=ToClrString(buffer);
		sprintf(buffer,"%s %s %s %s %s",who,ToAnsi(LocalizeText("Итем","Item")),s?s:itoa(r.ParamId,buf,10),GetOpName(false,r.Op).c_str(),r.ValueStr.c_str());
		text=ToClrString(buffer);
	}
	else if(r.Type==DR_LOCK)
	{
		text=LocalizeText("Блокировать ответ на ","Lock answer on ")+ToClrString(r.ValueStr)+LocalizeText(" минут"," minutes");
		name="_lock "+ToClrString(r.ValueStr);
	}
	else if(r.Type==DR_SCRIPT)
	{
		char values[1024]={0};
		if(r.ValuesCount==1) sprintf(values,"%s",r.ValuesNames[0].c_str());
		else if(r.ValuesCount==2) sprintf(values,"%s %s",r.ValuesNames[0].c_str(),r.ValuesNames[1].c_str());
		else if(r.ValuesCount==3) sprintf(values,"%s %s %s",r.ValuesNames[0].c_str(),r.ValuesNames[1].c_str(),r.ValuesNames[2].c_str());
		else if(r.ValuesCount==4) sprintf(values,"%s %s %s %s",r.ValuesNames[0].c_str(),r.ValuesNames[1].c_str(),r.ValuesNames[2].c_str(),r.ValuesNames[3].c_str());
		else if(r.ValuesCount==5) sprintf(values,"%s %s %s %s %s",r.ValuesNames[0].c_str(),r.ValuesNames[1].c_str(),r.ValuesNames[2].c_str(),r.ValuesNames[3].c_str(),r.ValuesNames[4].c_str());
		sprintf(buffer,"_script %s %d ",r.ParamName.c_str(),r.ValuesCount);
		strcat(buffer,values);
		name=ToClrString(buffer);
		sprintf(buffer,"%s %s (",ToAnsi(LocalizeText("Скрипт","Script")),r.ParamName.c_str());
		strcat(buffer,values);
		strcat(buffer,")");
		text=ToClrString(buffer);
	}
	else
		return nullptr;

	TreeNode^ r_n=r_node->Nodes->Add(name,text);
	r_n->ForeColor=System::Drawing::Color::Purple;
	return r_n;
}
#pragma endregion

#pragma region SaveTree
bool SaveTree(char* out_dlg)
{
	if(!out_dlg) return false;
	if(!(GetMainTree()->GetNodeCount(false) && GetMainTree()->Nodes->IndexOfKey("1")!=-1))
	{
		lErr->Text=LocalizeText("Нет диалогов.","No dialogs.");
		return false;
	}

	ostrstream stream(out_dlg,0x100000);

/************************************************************************/
/* Main                                                                 */
/************************************************************************/
	stream << "[comment]\n";
	stream << ToAnsi(rtComment->Text);
	stream << "\n\n";

/************************************************************************/
/* Main                                                                 */
/************************************************************************/
	stream << "[data]\n";
	stream << "lang=";
	for(int i=0,j=LangTree->Length;i<j;i++)
	{
		TreeView^ tree=LangTree[i];
		stream << ToAnsi(tree->Name);
		if(i!=j-1) stream << " ";
	}
	stream << "\n\n";
	if(stream.fail()) return false;

/************************************************************************/
/* Dialog                                                               */
/************************************************************************/
	stream << "[dialog]\n";
	System::Windows::Forms::TreeNode^ dlg_node;
	System::Windows::Forms::TreeNode^ link_node;

	dlg_node = GetMainTree()->Nodes->Find("1",false)[0];

	stream << "&\n";
	if(stream.fail()) return false;
	
	for(int i=1;i<=GetMainTree()->GetNodeCount(false);i++)
	{
		GetMainTree()->SelectedNode=dlg_node;

		// Save dialog data
		stream << ToAnsi(dlg_node->Name) << " " << i*1000 << " ";
		if(stream.fail()) return false;

		// доп. параметры диалога
		char tmp_str[300];
		sprintf(tmp_str,"%s",dlg_node->Name);
		int b_1=atoi(tmp_str);

		int flags=0;
		if(DialogOptions[b_1].DlgNoShuffle) flags|=DIALOG_FLAG_NO_SHUFFLE;
		stream << DialogOptions[b_1].DlgScript.c_str() << " " << flags << " ";
		if(stream.fail()) return false;

		if(dlg_node->GetNodeCount(false))
		{
			stream << "#\n";
			if(stream.fail()) return false;

			link_node=dlg_node->FirstNode;
			for(int j=1;j<=dlg_node->GetNodeCount(false);j++)
			{
				// Save dialog data
				stream << ToAnsi(link_node->Name) << " " << int(i*1000+j*10) << " ";
				if(stream.fail()) return false;

				if(link_node->GetNodeCount(false))
				{
					if(link_node->Nodes->IndexOfKey("demand")!=-1)
					{
						System::Windows::Forms::TreeNode^ node=link_node->Nodes->Find("demand",false)[0];
						if(node->Nodes->Count)
						{
							System::Windows::Forms::TreeNode^ node_=node->FirstNode;
							for(int l=0;l<node->GetNodeCount(false);l++)
							{
								stream << "D " << ToAnsi(node_->Name) << " ";
								if(stream.fail()) return false;
								node_=node_->NextNode;
							}
						}
					}
					if(link_node->Nodes->IndexOfKey("result")!=-1)
					{
						System::Windows::Forms::TreeNode^ node=link_node->Nodes->Find("result",false)[0];
						if(node->Nodes->Count)
						{
							System::Windows::Forms::TreeNode^ node_=node->FirstNode;
							for(int l=0;l<node->GetNodeCount(false);l++)
							{
								stream << "R " << ToAnsi(node_->Name) << " ";
								if(stream.fail()) return false;
								node_=node_->NextNode;
							}
						}
					}
				}

				if(j<dlg_node->GetNodeCount(false))
				{
					link_node=link_node->NextNode;
					stream << "#\n";
				}
			}
		}

		if(i==GetMainTree()->GetNodeCount(false))
			stream << "&\n"; // Последний диалог
		else
		{
			dlg_node=dlg_node->NextNode;
			stream << "@\n";
		}
		if(stream.fail()) return false;
	}
	stream << "\n";

/************************************************************************/
/* Texts                                                                */
/************************************************************************/
	for(int i=0,j=LangTree->Length;i<j;i++)
	{
		TreeView^ tree=LangTree[i];
		stream << "[" << ToAnsi(tree->Name) << "]\n";

		NpcInfo& info=GetNpcInfo(tree->Name);
		if(info.Name.length()) stream << "{100}{}{" << info.Name.c_str() << "}\n";
		if(info.Avatar.length()) stream << "{110}{}{" << info.Avatar.c_str() << "}\n";
		for(int i=0;i<8;i++)
		{
			StrVec& v=info.Info[i];
			for(int j=0;j<(int)v.size();j++) if(v[j].length()) stream << "{" << 200+i*10 << "}{}{" << v[j].c_str() << "}\n";
		}

		dlg_node=tree->Nodes->Find("1",false)[0];
		for(int b=1;b<=tree->GetNodeCount(false);b++)
		{
			tree->SelectedNode=dlg_node;

			stream << "{" << b*1000 << "}{}{" << ToAnsi(dlg_node->Text->Substring(dlg_node->Text->IndexOf(']')+2)) << "}\n";
			if(stream.fail()) return false;

			if(dlg_node->GetNodeCount(false))
			{
				link_node=dlg_node->FirstNode;
				for(int b2=1;b2<=dlg_node->GetNodeCount(false);b2++)
				{
					stream << "{" << b*1000+b2*10 << "}{}{" << ToAnsi(link_node->Text->Substring(link_node->Text->IndexOf(']')+2)) << "}\n";
					if(stream.fail()) return false;
					if(b2<dlg_node->GetNodeCount(false)) link_node=link_node->NextNode;
				}
			}
			if(b<tree->GetNodeCount(false)) dlg_node=dlg_node->NextNode;
		}

		for(UIntStrMulMap::iterator it=info.Texts.begin();it!=info.Texts.end();it++)
		{
			DWORD num=(*it).first;
			string& str=(*it).second;
			if(!str.length()) continue;
			stream << "{" << 100000000+num << "}{}{" << str.c_str() << "}\n";
		}

		stream << "\n";
	}

	return true;
/************************************************************************/
/*                                                                      */
/************************************************************************/
}
#pragma endregion

#pragma region Dialogs Text
private: System::Void btnReloadTexts_Click(System::Object^  sender, System::EventArgs^  e){}
void ReloadTexts(const char* name_lang){}
private: System::Void btnTextCheck_Click(System::Object^  sender, System::EventArgs^  e){}
void CheckMsg(FOMsg* msg, DialogPackMap* pack_map){}

#pragma endregion

#pragma region Load Dialog
private: System::Void btnDlgLoad_Click(System::Object^  sender, System::EventArgs^  e)
{
	OpenFileDialog^ dlg=gcnew OpenFileDialog;

	dlg->Filter="Dialogs files|*"+ToClrString(DIALOG_FILE_EXT);
	dlg->RestoreDirectory=true;

	if(dlg->ShowDialog()!=System::Windows::Forms::DialogResult::OK) return;
	if(DlgLoad(dlg->FileName)) lDlgName->Text=dlg->FileName;
}

bool DlgLoad(String^ name)
{
	lErr->Text="";
	FILE* f=fopen(ToAnsi(name),"rt");
	if(!f)
	{
		lErr->Text=LocalizeText("Ошибка при открытии файла.","Error open file.");
		return false;
	}

	fseek(f,0,SEEK_END);
	int len=ftell(f);
	fseek(f,0,SEEK_SET);

	char* buf=new char[len+1];
	if(!fread(buf,1,len,f))
	{
		delete[] buf;
		fclose(f);
		lErr->Text=LocalizeText("Ошибка при чтении файла.","Error read file.");
		return false;
	}
	fclose(f);
	buf[len]='\0';

	DialogPack* dlg_pack=DlgMngr.ParseDialog("dlg",1,buf);
	if(DlgMngr.LastErrors!="")
	{
		MessBox mb;
		mb.TxtBox->Text=ToClrString(DlgMngr.LastErrors);
		mb.ShowDialog();
	}

	if(!dlg_pack)
	{
		lErr->Text=LocalizeText("Ошибка парсинга диалога.","Unable to parse dialog.");
		delete[] buf;
		return false;
	}

	ResetDialog();
	for(int i=0,j=dlg_pack->Texts.size();i<j;i++)
	{
		String^ lang=ToClrString(dlg_pack->TextsLang[i].c_str());
		LoadTree(lang,dlg_pack->Dialogs,*dlg_pack->Texts[i]);
	}
	tcLang->SelectTab(0);
	UpdateDialog();
	rtComment->Text=ToClrString(dlg_pack->Comment);

	delete dlg_pack;
	delete[] buf;

	String^ str=name->Substring(name->LastIndexOf('\\')+1);
	this->Text=str->Substring(0,str->Length-strlen(DIALOG_FILE_EXT));
	return true;
}

bool DlgLoadOld(String^ name, DWORD id)
{
	lErr->Text="";
	FILE* f=fopen(ToAnsi(name),"rt");
	if(!f)
	{
		lErr->Text=LocalizeText("Ошибка открытия файла.","Error open file<"+name+">.");
		return false;
	}

	fseek(f,0,SEEK_END);
	int len=ftell(f);
	fseek(f,0,SEEK_SET);

	char* buf=new char[len+1];
	if(!fread(buf,1,len,f))
	{
		delete[] buf;
		fclose(f);
		lErr->Text="Error read file.";
		return false;
	}
	fclose(f);
	buf[len]='\0';

	DialogPack* dlg_pack=DlgMngr.ParseDialog("dlg",1,buf);
	if(!dlg_pack)
	{
		lErr->Text="Unable to parse dialog.";
		delete[] buf;
		return false;
	}

	dlg_pack->Comment="Ваши комментарии / Your comments.";

	static FOMsg* npc_r=NULL,*npc_e=NULL;
	if(!npc_r)
	{
		npc_r=new FOMsg;
		npc_e=new FOMsg;
		npc_r->LoadMsgFile(".\\dlg_from\\FONPC_R.MSG",PT_ROOT);
		npc_e->LoadMsgFile(".\\dlg_from\\FONPC_E.MSG",PT_ROOT);
	}

	dlg_pack->Texts.push_back(new FOMsg);
	dlg_pack->Texts.push_back(new FOMsg);
	dlg_pack->Texts[0]->LoadMsgFile(ToAnsi(name+"t"),PT_ROOT);
	dlg_pack->Texts[1]->LoadMsgFile(ToAnsi(name+"e"),PT_ROOT);
	dlg_pack->TextsLang.push_back("russ");
	dlg_pack->TextsLang.push_back("engl");

	FOMsg* m=dlg_pack->Texts[0];
	FOMsg* m2=npc_r;
	m->AddStr(100,string(m2->GetStr(id*1000)));
	if(m2->Count(id*1000+1)) m->AddStr(110,string(m2->GetStr(id*1000+1)));
	for(int i=0;i<8;i++)
		for(int l=0,k=m2->Count(id*1000+10+i);l<k;l++)
			m->AddStr(200+i*10,string(m2->GetStr(id*1000+10+i,l)));

	m=dlg_pack->Texts[1];
	m2=npc_e;
	m->AddStr(100,string(m2->GetStr(id*1000)));
	if(m2->Count(id*1000+1)) m->AddStr(110,string(m2->GetStr(id*1000+1)));
	for(int i=0;i<8;i++)
		for(int l=0,k=m2->Count(id*1000+10+i);l<k;l++)
			m->AddStr(200+i*10,string(m2->GetStr(id*1000+10+i,l)));

	ResetDialog();
	LoadTree("russ",dlg_pack->Dialogs,*dlg_pack->Texts[0]);
	LoadTree("engl",dlg_pack->Dialogs,*dlg_pack->Texts[1]);
	tcLang->SelectTab(0);
	UpdateDialog();
	rtComment->Text=ToClrString(dlg_pack->Comment);

	delete dlg_pack;
	delete[] buf;
	lErr->Text="Load complete.";
	return true;
}

#pragma endregion

#pragma region Save Dialog
private: System::Void btnDlgSave_Click_1(System::Object^  sender, System::EventArgs^  e)
{
	DlgSave(lDlgName->Text);
}

private: System::Void btnDlgSave_Click(System::Object^  sender, System::EventArgs^  e)
{
	SaveFileDialog^ dlg=gcnew SaveFileDialog;

	dlg->Filter="Dialogs files|*"+ToClrString(DIALOG_FILE_EXT);
	dlg->RestoreDirectory=true;

	if(dlg->ShowDialog()!=System::Windows::Forms::DialogResult::OK) return;
	lDlgName->Text=dlg->FileName;
	DlgSave(dlg->FileName);
}

void DlgSave(String^ name)
{
	lErr->Text="";
	if(name=="") return;

	//From Tree to Buffer
	char* buf=new char[0x100000];
	if(!SaveTree(buf))
	{
		lErr->Text=LocalizeText("Ошибка записи дерева в буффер.","Error parse tree to buffer.");
		delete[] buf;
		return;
	}

	//Save buf
	FILE* f;
	if(!(f=fopen(ToAnsi(name),"wt")))
	{
		lErr->Text=LocalizeText("Ошибка открытия файла при записи.","Unable to open file.");
		delete[] buf;
		return;
	}

	fwrite(buf,sizeof(char),strlen(buf),f);
	fclose(f);
	delete[] buf;

	String^ str=name->Substring(name->LastIndexOf('\\')+1);
	this->Text=str->Substring(0,str->Length-strlen(DIALOG_FILE_EXT));
}
#pragma endregion

#pragma region Vars Edit

private: System::Void btnVarChange_Click(System::Object^  sender, System::EventArgs^  e)
{
	lErr->Text="";
	if(!curVar)
	{
		lErr->Text=LocalizeText("Не выбрана переменная.","Var not select.");
		return;
	}

	if(curVar->TempId!=numVarId->Value)
	{
		lErr->Text=LocalizeText("Нельзя поменять id переменной.","Can't change id value.");
		return;
	}
	if(ToClrString(curVar->Name.c_str())!=txtVarName->Text)
	{
		lErr->Text=LocalizeText("Нельзя поменять имя у переменной.","Can't change name value.");
		return;
	}

	curVar->Desc=string(ToAnsi(txtVarDesc->Text));
	curVar->StartVal=(int)numVarStart->Value;
	curVar->MaxVal=(int)numVarMax->Value;
	curVar->MinVal=(int)numVarMin->Value;
	curVar->Flags=0;
	if(cbVarQuest->Checked==true) SETFLAG(curVar->Flags,VAR_FLAG_QUEST);
	if(cbVarRandom->Checked==true) SETFLAG(curVar->Flags,VAR_FLAG_RANDOM);
	if(cbVarNoCheck->Checked==true) SETFLAG(curVar->Flags,VAR_FLAG_NO_CHECK);
	if(rbVarGlobal->Checked==true) curVar->Type=VAR_GLOBAL;
	else if(rbVarLocal->Checked==true) curVar->Type=VAR_LOCAL;
	else if(rbVarUnicum->Checked==true) curVar->Type=VAR_UNICUM;
	else if(rbVarLocalLocation->Checked==true) curVar->Type=VAR_LOCAL_LOCATION;
	else if(rbVarLocalMap->Checked==true) curVar->Type=VAR_LOCAL_MAP;
	else curVar->Type=VAR_LOCAL_ITEM;
	VarMngr.SaveTemplateVars();
}

void ResetVar()
{

}

void UpdateVarsList()
{
	TempVarVec& tvars=VarMngr.GetTemplateVars();
	lbAllVars->Items->Clear();
	for(TempVarVec::iterator it=tvars.begin();it!=tvars.end();++it)
	{
		TemplateVar* tvar=*it;
		if(tvar) lbAllVars->Items->Add(((Int32)tvar->TempId).ToString()+" - "+ToClrString(tvar->Name.c_str()));
	}
	cbDemandVars->Items->Clear();
	for(TempVarVec::iterator it=tvars.begin();it!=tvars.end();++it)
	{
		TemplateVar* tvar=*it;
		if(tvar) cbDemandVars->Items->Add(ToClrString(tvar->Name.c_str()));
	}
	cbResultVars->Items->Clear();
	for(TempVarVec::iterator it=tvars.begin();it!=tvars.end();++it)
	{
		TemplateVar* tvar=*it;
		if(tvar) cbResultVars->Items->Add(ToClrString(tvar->Name.c_str()));
	}
}

private: System::Void lbVars_MouseDoubleClick(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e)
{
	int num=lbAllVars->SelectedIndex;
	if(num==-1) return;

	TemplateVar* tvar=NULL;
	TempVarVec& tvars=VarMngr.GetTemplateVars();
	int cnt=0;
	for(TempVarVec::iterator it=tvars.begin();it!=tvars.end();++it)
	{
		TemplateVar* tvar_=*it;
		if(tvar_)
		{
			if(cnt==num)
			{
				tvar=tvar_;
				break;
			}
			cnt++;
		}
	}
	if(!tvar) return;
	curVar=tvar;

	numVarId->Value=curVar->TempId;
	txtVarName->Text=ToClrString(curVar->Name.c_str());
	txtVarDesc->Text=ToClrString(curVar->Desc.c_str());
	numVarStart->Value=curVar->StartVal;
	numVarMax->Value=curVar->MaxVal;
	numVarMin->Value=curVar->MinVal;

	cbVarQuest->Checked=false;
	cbVarRandom->Checked=false;
	cbVarNoCheck->Checked=false;
	if(FLAG(curVar->Flags,VAR_FLAG_QUEST)) cbVarQuest->Checked=true;
	if(FLAG(curVar->Flags,VAR_FLAG_RANDOM)) cbVarRandom->Checked=true;
	if(FLAG(curVar->Flags,VAR_FLAG_NO_CHECK)) cbVarNoCheck->Checked=true;

	if(curVar->Type==VAR_GLOBAL) rbVarGlobal->Checked=true;
	else if(curVar->Type==VAR_LOCAL) rbVarLocal->Checked=true;
	else if(curVar->Type==VAR_UNICUM) rbVarUnicum->Checked=true;
	else if(curVar->Type==VAR_LOCAL_LOCATION) rbVarLocalLocation->Checked=true;
	else if(curVar->Type==VAR_LOCAL_MAP) rbVarLocalMap->Checked=true;
	else rbVarLocalItem->Checked=true; // VAR_LOCAL_ITEM
}

private: System::Void txtVarDesc_TextChanged(System::Object^  sender, System::EventArgs^  e)
{
}

private: System::Void numVarStart_ValueChanged(System::Object^  sender, System::EventArgs^  e)
{
}

private: System::Void numVarMax_ValueChanged(System::Object^  sender, System::EventArgs^  e)
{
}

private: System::Void numVarMin_ValueChanged(System::Object^  sender, System::EventArgs^  e)
{
}

private: System::Void txtVarName_TextChanged(System::Object^  sender, System::EventArgs^  e)
{
}

private: System::Void numVarId_ValueChanged(System::Object^  sender, System::EventArgs^  e)
{
}

private: System::Void btnVarAdd_Click(System::Object^  sender, System::EventArgs^  e)
{
	lErr->Text="";
	TemplateVar* var=new TemplateVar;

	if(rbVarGlobal->Checked==true) var->Type=VAR_GLOBAL;
	else if(rbVarLocal->Checked==true) var->Type=VAR_LOCAL;
	else if(rbVarUnicum->Checked==true) var->Type=VAR_UNICUM;
	else if(rbVarLocalLocation->Checked==true) var->Type=VAR_LOCAL_LOCATION;
	else if(rbVarLocalMap->Checked==true) var->Type=VAR_LOCAL_MAP;
	else if(rbVarLocalItem->Checked==true) var->Type=VAR_LOCAL_ITEM;
	else return;

	var->TempId=(UINT)numVarId->Value;
	var->Name=string(ToAnsi(txtVarName->Text));

	var->Desc=string(ToAnsi(txtVarDesc->Text));

	var->StartVal=(int)numVarStart->Value;
	var->MaxVal=(int)numVarMax->Value;
	var->MinVal=(int)numVarMin->Value;

	var->Flags=0;
	if(cbVarQuest->Checked==true) SETFLAG(var->Flags,VAR_FLAG_QUEST);
	if(cbVarRandom->Checked==true) SETFLAG(var->Flags,VAR_FLAG_RANDOM);
	if(cbVarNoCheck->Checked==true) SETFLAG(var->Flags,VAR_FLAG_NO_CHECK);

	if(VarMngr.AddTemplateVar(var))
	{
		VarMngr.SaveTemplateVars();
		UpdateVarsList();
	}
	else
	{
		delete var;
		lErr->Text=LocalizeText("Ошибка при добавлении переменной.","Add var fail.");
	}
}

private: System::Void btnVarDelete_Click(System::Object^  sender, System::EventArgs^  e)
{
	WORD tid=(UINT)numVarId->Value;
	VarMngr.EraseTemplateVar(tid);
	VarMngr.SaveTemplateVars();
	UpdateVarsList();
}

#pragma endregion

private: System::Void tabDialogsMenu_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void lbDialogsList_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void btnResultEdit_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void btnDialogCheck_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void dataGridView1_CellContentClick(System::Object^  sender, System::Windows::Forms::DataGridViewCellEventArgs^  e){}
private: System::Void tcLang_KeyPress(System::Object^  sender, System::Windows::Forms::KeyPressEventArgs^  e){}

private: System::Void tcLang_KeyUp(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e){}

private: System::Void tcLang_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e)
{
	/*if(Control::ModifierKeys != Keys::Control) return;

	TreeView^ tree=GetCurTree();
	if(!tree) return;
	TreeNode^ node=tree->SelectedNode;

	if(e->KeyCode==Keys::X || e->KeyCode==Keys::C)
	{
		//if(tree->SelectedNode->Level<2 && e->Button==System::Windows::Forms::MouseButtons::Right) EditNode(tree,tree!=GetMainTree());

		Clipboard::SetText("de>"+node->Text);
	}
	else if(e->KeyCode==Keys::V)
	{
		String^ text=Clipboard::GetText();
		if(text->Length<3 || text[0]!='d' || text[1]!='e' || text[2]!='>') return;

		node->Text=text->Substring(3,text->Length-3);
	}*/
}
private: System::Void btnDemandEdit_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void radioButton7_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void rbResultScript_CheckedChanged(System::Object^  sender, System::EventArgs^  e)
{
	if(rbResultScript->Checked)
	{
		gbResultScript->Visible=true;
		cbResultValue->Visible=false;
	}
	else
	{
		gbResultScript->Visible=false;
		cbResultValue->Visible=true;
	}
}
private: System::Void rbDemandScript_CheckedChanged(System::Object^  sender, System::EventArgs^  e)
{
	if(rbDemandScript->Checked)
	{
		gbDemandScript->Visible=true;
		cbDemandValue->Visible=false;
	}
	else
	{
		gbDemandScript->Visible=false;
		cbDemandValue->Visible=true;
	}
}

};
}
