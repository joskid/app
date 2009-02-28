/***************************************************************************
 SocNetV: Social Networks Visualiser 
 version: 0.52
 Written in Qt 4.4

                           mainwindow.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2009 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
 ***************************************************************************/

/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtGui>
#include <QtDebug>


#include "mainwindow.h"
#include "graphicswidget.h"
#include "node.h"
#include "edge.h"
#include "nodenumber.h"
#include "nodelabel.h"
#include "edgeweight.h"
#include "htmlviewer.h"
#include "texteditor.h"
#include "backgrcircle.h"
#include "vertex.h"




bool printDebug=FALSE;


void myMessageOutput( QtMsgType type, const char *msg )     {
	if (printDebug)	
		switch ( type ) {
			case QtDebugMsg:
				fprintf( stderr, "Debug: %s\n", msg );
				break;
			case QtWarningMsg:
				fprintf( stderr, "Warning: %s\n", msg );
				break;
			case QtFatalMsg:
				fprintf( stderr, "Fatal: %s\n", msg );
				abort();                    // deliberately core dump
			case QtCriticalMsg:
				fprintf( stderr, "Critical: %s\n", msg );
				abort();                    // deliberately core dump
		}
}


/** MainWindow contruction method **/
MainWindow::MainWindow(const QString &fName) {
	fileName=fName;
	qInstallMsgHandler( myMessageOutput );
	setWindowIcon (QIcon(":/images/socnetv.png"));
	VERSION="0.52";

	/** inits that invoke all other construction parts **/
	initActions();  //register and construct menu Actions 
	initMenuBar();  //construct menu
	initToolBar();  //build the toolbar
	
	initStatusBar();  //and now add the status bar.

	initToolBox(); //finally, build the toolbox

	colorList = QColor::colorNames();  //and fill a stringList with all X-supported color names

	//set MW minimum size, before creating scene and canvas, so that we may have a clue about their sizes..
	this->setMinimumSize(800,600);

 	initView(); //create the canvas

	//Connect some signals to/from the canvas and the Graph
	connect( graphicsWidget, SIGNAL( selectedNode(Node*) ), this, SLOT( nodeInfoStatusBar(Node*) ) );
	connect( graphicsWidget, SIGNAL( selectedEdge(Edge*) ), this, SLOT ( linkInfoStatusBar(Edge*) ) );
	connect( graphicsWidget, SIGNAL( windowResized(int, int)),this, SLOT( windowInfoStatusBar(int, int)) );  

	connect( graphicsWidget, SIGNAL( userDoubleClicked(int, QPointF) ), this, SLOT( addNodeWithMouse(int, QPointF) ) ) ;
	connect( graphicsWidget, SIGNAL( userMiddleClicked(int, int, float) ), this, SLOT( addLink(int, int, float) ) );
	connect( graphicsWidget, SIGNAL( openNodeMenu() ), this, SLOT(openNodeContextMenu() ) ) ;
	connect( graphicsWidget, SIGNAL( openEdgeMenu() ), this, SLOT(openLinkContextMenu() ) ) ;
	connect( graphicsWidget, SIGNAL(updateNodeCoords(int, int, int)), this, SLOT(updateNodeCoords(int, int, int)) );
	connect( graphicsWidget, SIGNAL(zoomChanged(int)),zoomCombo, SLOT(setCurrentIndex(int)));

	connect( &activeGraph, SIGNAL( addBackgrCircle (int, int, int) ), graphicsWidget, SLOT(addBackgrCircle(int, int, int) ) ) ;
	connect( &activeGraph, SIGNAL( addBackgrHLine (int) ), graphicsWidget, SLOT(addBackgrHLine(int) ) ) ;
	connect( &activeGraph, SIGNAL( moveNode(int, int, int) ), graphicsWidget, SLOT(moveNode(int, int, int) ) ) ;
	connect( &activeGraph, SIGNAL( drawNode( int ,int,  QString, QString, QString, QPointF, QString, bool, bool) ), graphicsWidget, SLOT( drawNode( int ,int,  QString, QString, QString, QPointF, QString, bool, bool)  ) ) ;
	connect( &activeGraph, SIGNAL( eraseEdge(int, int)), graphicsWidget, SLOT( eraseEdge(int, int) ) );
	connect( &activeGraph, SIGNAL( graphChanged() ), this, SLOT( graphChanged() ) ) ;

	connect( &activeGraph, SIGNAL( drawEdge( int, int, float, bool, bool, QString, bool, bool)), graphicsWidget, SLOT( drawEdge( int, int,float, bool, bool, QString, bool, bool  ) )  ) ;

	connect( &activeGraph, SIGNAL( drawEdgeReciprocal(int, int) ),  graphicsWidget, SLOT( drawEdgeReciprocal(int, int) ) );
	

	//connect some signals/slots with MW widgets
	connect (addNodeBt,SIGNAL(clicked()), this, SLOT(addNode()));
	connect (addLinkBt,SIGNAL(clicked()), this, SLOT(slotAddLink()));
	connect (removeNodeBt,SIGNAL(clicked()), this, SLOT(slotRemoveNode()));
	connect (removeLinkBt,SIGNAL(clicked()), this, SLOT(slotRemoveLink()));

	connect(zoomCombo, SIGNAL(currentIndexChanged(const int &)),graphicsWidget, SLOT(changeZoom(const int &)));
	connect(zoomOutAct, SIGNAL(triggered()), graphicsWidget, SLOT(zoomOut()));
	connect(zoomInAct, SIGNAL(triggered()), graphicsWidget, SLOT(zoomIn()));

	connect(rotateSpinBox, SIGNAL(valueChanged(int)), graphicsWidget, SLOT(rot(int) ) );
	connect(circleClearBackgrCirclesAct, SIGNAL(activated()), graphicsWidget, SLOT(clearBackgrCircles()));
 
	//create an horizontal layout for the toolbox and the canvas. This will be our MW layout.
	QHBoxLayout *layout = new QHBoxLayout;
	//add them 
	layout->addWidget(toolBox);
	layout->addWidget(graphicsWidget);
	//create a dummy widget, for the above layout
	QWidget *widget = new QWidget;
	widget->setLayout(layout);
	//now set this as central widget of MW
	setCentralWidget(widget);

	//initialise default network parameters
	initNet();

	activeGraph.setParent(this);	//Used by random-network creation methods to update ASAP the view

	/** DEFAULTING HERE DOES NOT CHANGE BOOL VALUE **/
	/** EVERY TIME INITNET IS CALLED **/
	bezier=FALSE; 
	firstTime=TRUE;
	showProgressBar=FALSE;	

	graphicsWidget->setInitNodeColor(initNodeColor);	
	graphicsWidget->setInitNumberDistance(numberDistance);
	graphicsWidget->setInitLabelDistance(labelDistance);
	graphicsWidget->setInitNodeSize(initNodeSize);
	graphicsWidget->setBackgroundBrush(QBrush(initBackgroundColor)); //Qt::gray

	 /**Load file one exec time*/
	if (!fileName.isEmpty())     {
                 fileNameNoPath=fileName.split ("/");
                this->loadNetworkFile(fileName);
      	}

	if (firstTime) {
		createFortuneCookies();	
		createTips();
	}	

	statusBar()->showMessage(tr("Welcome to Social Networks Visualiser, Version ")+VERSION, statusBarDuration);
}



MainWindow::~MainWindow() {
	delete printer;
	delete scene;
	delete graphicsWidget;
}



/** initializes all QActions of the application */
void MainWindow::initActions(){
	printer = new QPrinter;
	
	/**
	File menu actions
	*/
	fileNew = new QAction(QIcon(":/images/new.png"), tr("&New"),  this);
	fileNew->setShortcut(tr("Ctrl+N"));
	fileNew->setStatusTip(tr("Creates a new network"));
	fileNew->setToolTip(tr("New network (Ctrl+N)"));
	fileNew->setWhatsThis(tr("New\n\nCreates a new network"));
	connect(fileNew, SIGNAL(activated()), this, SLOT(slotCreateNew()));

	fileOpen = new QAction(QIcon(":/images/open.png"), tr("&Open"), this);
	fileOpen->setShortcut(tr("Ctrl+O"));
	fileOpen->setToolTip(tr("Open network (Ctrl+O)"));
	fileOpen->setStatusTip(tr("Opens a a file of an existing network"));
	fileOpen->setWhatsThis(tr("Open\n\nOpens a file of an existing network"));
	connect(fileOpen, SIGNAL(activated()), this, SLOT(slotChooseFile()));

  	fileSave = new QAction(QIcon(":/images/save.png"), tr("&Save"),  this);
	fileSave->setShortcut(tr("Ctrl+S"));
	fileSave->setToolTip(tr("Save network (Ctrl+S)"));
	fileSave->setStatusTip(tr("Saves the actual network to the current file"));
	fileSave->setWhatsThis(tr("Save.\n\nSaves the actual network"));
	connect(fileSave, SIGNAL(activated()), this, SLOT(slotFileSave()));

	fileSaveAs = new QAction(QIcon(":/images/save.png"), tr("Save &As..."),  this);
	fileSaveAs->setShortcut(tr("Ctrl+Shift+S"));
	fileSaveAs->setStatusTip(tr("Saves the actual network under a new filename"));
	fileSaveAs->setWhatsThis(tr("Save As\n\nSaves the actual network under a new filename"));
	connect(fileSaveAs, SIGNAL(activated()), this, SLOT(slotFileSaveAs()));

	exportBMP = new QAction(QIcon(":/images/save.png"), tr("&BMP..."), this);
	exportBMP->setStatusTip(tr("Export network to a BMP image"));
	exportBMP->setWhatsThis(tr("Export BMP \n\n Export network to a BMP image"));
	connect(exportBMP, SIGNAL(activated()), this, SLOT(slotExportBMP()));

	exportPNG = new QAction( QIcon(":/images/save.png"), tr("&PNG..."), this);
	exportPNG->setStatusTip(tr("Export network to a PNG image"));
	exportPNG->setWhatsThis(tr("Export PNG \n\n Export network to a PNG image"));
	connect(exportPNG, SIGNAL(activated()), this, SLOT(slotExportPNG()));


	exportPDF = new QAction( QIcon(":/images/save.png"), tr("&PDF..."), this);
	exportPDF->setStatusTip(tr("Export network to a PDF file"));
	exportPDF->setWhatsThis(tr("Export PDF\n\n Export network to a PDF document"));
	connect(exportPDF, SIGNAL(activated()), this, SLOT(slotExportPDF()));

	exportSM = new QAction( QIcon(":/images/save.png"), tr("&Adjacency Matrix"), this);
	exportSM->setStatusTip(tr("Export network to an adjacency matrix file"));
	exportSM->setWhatsThis(tr("Export Sociomatrix \n\n Export network to a adjacency matrix-formatted file"));
	connect(exportSM, SIGNAL(activated()), this, SLOT(slotExportSM()));

	exportPajek = new QAction( QIcon(":/images/save.png"), tr("&Pajek"), this);
	exportPajek->setStatusTip(tr("Export network to a Pajek-formatted file"));
	exportPajek->setWhatsThis(tr("Export Pajek \n\n Export network to a Pajek-formatted file"));
	connect(exportPajek, SIGNAL(activated()), this, SLOT(slotExportPajek()));

	exportList = new QAction( QIcon(":/images/save.png"), tr("&List"), this);
	exportList->setStatusTip(tr("Export network to a List-formatted file. "));
	exportList->setWhatsThis(tr("Export List\n\nExport network to a List-formatted file"));
	connect(exportList, SIGNAL(activated()), this, SLOT(slotExportList()));

	exportDL = new QAction( QIcon(":/images/save.png"), tr("&DL..."), this);
	exportDL->setStatusTip(tr("Export network to a DL-formatted file"));
	exportDL->setWhatsThis(tr("Export DL\n\nExport network to a DL-formatted"));
	connect(exportDL, SIGNAL(activated()), this, SLOT(slotExportDL()));

	exportGW = new QAction( QIcon(":/images/save.png"), tr("&GW..."), this);
	exportGW->setStatusTip(tr("Export network to a GW-formatted file"));
	exportGW->setWhatsThis(tr("Export\n\nExport network to a GW formatted file"));
	connect(exportGW, SIGNAL(activated()), this, SLOT(slotExportGW()));

	fileClose = new QAction( tr("&Close"), this);
	fileClose->setStatusTip(tr("Closes the actual network"));
	fileClose->setWhatsThis(tr("Close \n\nCloses the actual network"));
	connect(fileClose, SIGNAL(activated()), this, SLOT(slotFileClose()));

	printNetwork = new QAction(QIcon(":/images/print.png"), tr("&Print"), this);
	printNetwork->setShortcut(tr("Ctrl+P"));
	printNetwork->setStatusTip(tr("Prints whatever is viewable on the canvas."));
	printNetwork->setWhatsThis(tr("Printing \n\n This function prints whatever is viewable on the canvas. \nTo print the whole network, you might want to zoom-out."));
	connect(printNetwork, SIGNAL(activated()), this, SLOT(slotPrintView()));

	fileQuit = new QAction(QIcon(":/images/exit.png"), tr("E&xit"), this);
	fileQuit->setShortcut(tr("Ctrl+Q"));
	fileQuit->setStatusTip(tr("Quits the application"));
	fileQuit->setWhatsThis(tr("Exit\n\nQuits the application"));
	connect(fileQuit, SIGNAL(activated()), this, SLOT(close()));

	viewNetworkFileAct = new QAction(QIcon(":/images/net2.png"), tr("View Loaded File"),this);
	viewNetworkFileAct ->setShortcut(tr("F5"));
	viewNetworkFileAct->setStatusTip(tr("Displays the loaded network file"));
	viewNetworkFileAct->setWhatsThis(tr("View Loaded File\n\nDisplays the file of the loaded network"));
	connect(viewNetworkFileAct, SIGNAL(activated()), this, SLOT(slotViewNetworkFile()));

	viewSociomatrixAct = new QAction(QIcon(":/images/sm.png"), tr("View Adjacency Matrix"),  this);
	viewSociomatrixAct ->setShortcut(tr("F6"));
	viewSociomatrixAct->setStatusTip(tr("Displays the adjacency matrix of the active network"));
	viewSociomatrixAct->setWhatsThis(tr("View Network file\n\nDisplays the adjacency matrix of the active network"));
	connect(viewSociomatrixAct, SIGNAL(activated()), this, SLOT(slotViewAdjacencyMatrix()));

	createUniformRandomNetworkAct = new QAction(QIcon(":/images/erdos.png"), tr("Erdos-Renyi G(n,p)"),  this);
	createUniformRandomNetworkAct ->setShortcut(tr("Shift+U"));
	createUniformRandomNetworkAct->setStatusTip(tr("Creates a random network where each edge is included with a given probability"));
	createUniformRandomNetworkAct->setWhatsThis(tr("Uniform \n\nCreates a random network of G(n, p) model by connecting nodes randomly. Each edge is included in the graph with equal probability p, independently of the other edges"));
	connect(createUniformRandomNetworkAct, SIGNAL(activated()), this, SLOT(slotCreateRandomNetErdos()));

	createConnectedRandomNetworkAct = new QAction( tr("Connected"),  this);
	createConnectedRandomNetworkAct->setStatusTip(tr("Creates a connected random network"));
	createConnectedRandomNetworkAct->setWhatsThis(tr("Uniform Connected\n\nCreates a connected random network "));
	connect(createConnectedRandomNetworkAct, SIGNAL(activated()), this, SLOT(slotCreateConnectedRandomNetwork()));

	createLatticeNetworkAct = new QAction( QIcon(":/images/net1.png"), tr("Ring Lattice"), this);
	createLatticeNetworkAct ->setShortcut(tr("Shift+L"));
	createLatticeNetworkAct->setStatusTip(tr("Creates a ring lattice random network"));
	createLatticeNetworkAct->setWhatsThis(tr("Ring Lattice \n\nA ring lattice or a physicist's lattice is a graph with N nodes each connected to K neighbors, K / 2 on each side."));
	connect(createLatticeNetworkAct, SIGNAL(activated()), this, SLOT(slotCreateRandomNetRingLattice()));

	createSameDegreeRandomNetworkAct = new QAction(QIcon(":/images/net.png"), tr("Same Degree"), this);
	createSameDegreeRandomNetworkAct->setStatusTip(tr("Creates a random network where all nodes have the same degree."));
	createSameDegreeRandomNetworkAct->setWhatsThis(tr("Same Degree \n\nCreates a random network where all nodes have the same degree "));
	connect(createSameDegreeRandomNetworkAct, SIGNAL(activated()), this, SLOT(slotCreateSameDegreeRandomNetwork()));

	createGaussianRandomNetworkAct = new QAction(tr("Gaussian"),	this);
	createGaussianRandomNetworkAct->setStatusTip(tr("Creates a Gaussian distributed random network"));
	createGaussianRandomNetworkAct->setWhatsThis(tr("Gaussian \n\nCreates a random network of Gaussian distribution"));
	connect(createGaussianRandomNetworkAct, SIGNAL(activated()), this, SLOT(slotCreateGaussianRandomNetwork()));

	createSmallWorldRandomNetworkAct = new QAction(QIcon(":/images/sw.png"), tr("Small World"),	this);
	createSmallWorldRandomNetworkAct->setShortcut(tr("Shift+W"));
	createSmallWorldRandomNetworkAct->setStatusTip(tr("Creates a random network with small world properties"));
	createSmallWorldRandomNetworkAct->setWhatsThis(tr("Small World \n\nA Small World, according to the Watts and Strogatz model, is a random network with short average path lengths and high clustering coefficient."));
	connect(createSmallWorldRandomNetworkAct, SIGNAL(activated()), this, SLOT(slotCreateSmallWorldRandomNetwork()));

	
	/**
	Edit menu actions
	*/
	findNodeAct = new QAction(QIcon(":/images/find.png"), tr("Find Node"), this);
	findNodeAct->setShortcut(tr("Ctrl+F"));
	findNodeAct->setStatusTip(tr("Finds and highlights a node by number or label. Press Ctrl+F again to undo."));
	findNodeAct->setWhatsThis(tr("Find Node\n\nFinds a node with a given number or label and doubles its size. Ctrl+F again resizes back the node"));
	connect(findNodeAct, SIGNAL(activated()), this, SLOT(slotFindNode()) );
		
	addNodeAct = new QAction(QIcon(":/images/add.png"), tr("Add Node"), this);
	addNodeAct->setShortcut(tr("Ctrl+A"));
	addNodeAct->setStatusTip(tr("Adds a node"));
	addNodeAct->setWhatsThis(tr("Add Node\n\nAdds a node to the network"));
	connect(addNodeAct, SIGNAL(activated()), this, SLOT(addNode()));

	removeNodeAct = new QAction(QIcon(":/images/remove.png"),tr("Remove Node"), this);
	removeNodeAct ->setShortcut(tr("Ctrl+Shift+A"));
	removeNodeAct->setStatusTip(tr("Removes a node"));
	removeNodeAct->setWhatsThis(tr("Remove Node\n\nRemoves a node from the network"));
	connect(removeNodeAct, SIGNAL(activated()), this, SLOT(slotRemoveNode()));

	changeNodeLabelAct = new QAction(QIcon(":/images/letters.png"), tr("Change Label"),	this);
	changeNodeLabelAct->setStatusTip(tr("Changes the Label of a node"));
	changeNodeLabelAct->setWhatsThis(tr("Change Label\n\nChanges the label of a node"));
	connect(changeNodeLabelAct, SIGNAL(activated()), this, SLOT(slotChangeNodeLabel()));

	changeNodeColorAct = new QAction(QIcon(":/images/colorize.png"), tr("Change Color"), this);
	changeNodeColorAct->setStatusTip(tr("Changes the color of a node"));
	changeNodeColorAct->setWhatsThis(tr("Change Color\n\nChanges the Color of a node"));
	connect(changeNodeColorAct, SIGNAL(activated()), this, SLOT(slotChangeNodeColor()));

	changeNodeSizeAct = new QAction(QIcon(":/images/resize.png"),tr("Change Size"), this);
	changeNodeSizeAct->setStatusTip(tr("Changes the actual size of a node"));
	changeNodeSizeAct->setWhatsThis(tr("Change Size\n\nChanges the actual size of a node"));
	connect(changeNodeSizeAct, SIGNAL(activated()), this, SLOT(slotChangeNodeSize()));

	changeNodeValueAct = new QAction(tr("Change Value"), this);
	changeNodeValueAct->setStatusTip(tr("Changes the value of a node"));
	changeNodeValueAct->setWhatsThis(tr("Change Value\n\nChanges the value of a node"));
	connect(changeNodeValueAct, SIGNAL(activated()), this, SLOT(slotChangeNodeValue()));

	changeAllNodesSizeAct = new QAction(QIcon(":/images/resize.png"), tr("Change all Nodes Size"),	this);
	changeAllNodesSizeAct->setStatusTip(tr("This option lets you change the size of all nodes"));
	changeAllNodesSizeAct->setWhatsThis(tr("Nodes Size\n\nThis option lets you change the size of all nodes"));
	connect(changeAllNodesSizeAct, SIGNAL(activated()), this, SLOT(slotChangeAllNodesSize()) );

	changeAllNodesShapeAct = new QAction( tr("Change all Nodes Shape"),	this);
	changeAllNodesShapeAct->setStatusTip(tr("This option lets you change the shape of all nodes"));
	changeAllNodesShapeAct->setWhatsThis(tr("Nodes Shape\n\nThis option lets you change the shape of all nodes"));
	connect(changeAllNodesShapeAct, SIGNAL(activated()), this, SLOT(slotChangeAllNodesShape()) );

	changeNodeBoxAct = new QAction(QIcon(":/images/box.png"), tr("Change Node Shape to Box"),this);
	changeNodeBoxAct->setStatusTip(tr("This option lets you change the shape of a node to a box"));
	changeNodeBoxAct->setWhatsThis(tr("Node as a box\n\nThis option lets you change the shape of a node to a box"));
	connect(changeNodeBoxAct, SIGNAL(activated()), this, SLOT(slotChangeNodeBox()) );

	changeNodeTriangleAct = new QAction( tr("Change Node Shape to Triangle"),	this);
	changeNodeTriangleAct->setStatusTip(tr("This option lets you change the shape of a node to a box"));
	changeNodeTriangleAct->setWhatsThis(tr("Node as a box\n\nThis option lets you change the shape of a node to a box"));
	connect(changeNodeTriangleAct, SIGNAL(activated()), this, SLOT(slotChangeNodeTriangle()) );

	changeNodeCircleAct = new QAction(QIcon(":/images/circle.png"), tr("Change Node Shape to Circle"),	this);
	changeNodeCircleAct->setStatusTip(tr("This option lets you change the shape of a node to a box"));
	changeNodeCircleAct->setWhatsThis(tr("Node as a box\n\nThis option lets you change the shape of a node to a box"));
	connect(changeNodeCircleAct, SIGNAL(activated()), this, SLOT(slotChangeNodeCircle()) );

	changeNodeDiamondAct = new QAction(QIcon(":/images/diamond.png"), tr("Change Node Shape to Diamond"),	this);
	changeNodeDiamondAct->setStatusTip(tr("This option lets you change the shape of a node to a box"));
	changeNodeDiamondAct->setWhatsThis(tr("Node as a box\n\nThis option lets you change the shape of a node to a box"));
	connect(changeNodeDiamondAct, SIGNAL(activated()), this, SLOT(slotChangeNodeDiamond()) );

	changeNodeEllipseAct = new QAction( tr("Change Node Shape to Ellipse"),	this);
	changeNodeEllipseAct->setStatusTip(tr("This option lets you change the shape of a node to a box"));
	changeNodeEllipseAct->setWhatsThis(tr("Node as a box\n\nThis option lets you change the shape of a node to a box"));
	connect(changeNodeEllipseAct, SIGNAL(activated()), this, SLOT(slotChangeNodeEllipse()) );

	changeNumbersSizeAct = new QAction( tr("Change all Numbers Size"),	this);
	changeNumbersSizeAct->setStatusTip(tr("It lets you change the font size of the numbers of all nodes"));
	changeNumbersSizeAct->setWhatsThis(tr("Numbers Size\n\nChanges the size of the numbers of all nodes"));
	connect(changeNumbersSizeAct, SIGNAL(activated()), this, SLOT(slotChangeNumbersSize()) );

	changeLabelsSizeAct = new QAction( tr("Change all Labels Size"), this);
	changeLabelsSizeAct->setStatusTip(tr("You can change the font size of the labels of all nodes"));
	changeLabelsSizeAct->setWhatsThis(tr("Labels Size\n\nChange the fontsize of the labels of all nodes"));
	connect(changeLabelsSizeAct, SIGNAL(activated()), this, SLOT(slotChangeLabelsSize()) );
	
	addLinkAct = new QAction(QIcon(":/images/plines.png"), tr("Add Link"),this);
	addLinkAct->setShortcut(tr("Ctrl+L"));
	addLinkAct->setStatusTip(tr("Adds a Link to a Node"));
	addLinkAct->setWhatsThis(tr("Add Link\n\nAdds a Link to the network"));
	connect(addLinkAct, SIGNAL(activated()), this, SLOT(slotAddLink()));
	
	removeLinkAct = new QAction(QIcon(":/images/disconnect.png"), tr("Remove"), this);
	removeLinkAct ->setShortcut(tr("Ctrl+Shift+L"));
	removeLinkAct->setStatusTip(tr("Removes a Link"));
	removeLinkAct->setWhatsThis(tr("Remove Link\n\nRemoves a Link from the network"));
	connect(removeLinkAct, SIGNAL(activated()), this, SLOT(slotRemoveLink()));
	
	changeLinkLabelAct = new QAction(QIcon(":/images/letters.png"), tr("Change Label"), this);
	changeLinkLabelAct->setStatusTip(tr("Changes the Label of a Link"));
	changeLinkLabelAct->setWhatsThis(tr("Change Label\n\nChanges the label of a Link"));
	connect(changeLinkLabelAct, SIGNAL(activated()), this, SLOT(slotChangeLinkLabel()));
	changeLinkLabelAct->setEnabled(false);

	changeLinkColorAct = new QAction(QIcon(":/images/colorize.png"),tr("Change Color"),	this);
	changeLinkColorAct->setStatusTip(tr("Changes the Color of a Link"));
	changeLinkColorAct->setWhatsThis(tr("Change Color\n\nChanges the Color of a Link"));
	connect(changeLinkColorAct, SIGNAL(activated()), this, SLOT(slotChangeLinkColor()));

	changeLinkWeightAct = new QAction(tr("Change Weight"), this);
	changeLinkWeightAct->setStatusTip(tr("Changes the Weight of a Link"));
	changeLinkWeightAct->setWhatsThis(tr("Change Value\n\nChanges the Weight of a Link"));
	connect(changeLinkWeightAct, SIGNAL(activated()), this, SLOT(slotChangeLinkWeight()));

	filterNodesAct = new QAction(tr("Filter Nodes"), this);
	filterNodesAct -> setEnabled(false);
	filterNodesAct->setStatusTip(tr("Filters Nodes of some value out of the network"));
	filterNodesAct->setWhatsThis(tr("Filter Nodes\n\nFilters Nodes of some value out of the network."));
	connect(filterNodesAct, SIGNAL(activated()), this, SLOT(slotFilterNodes()));

	filterLinksAct = new QAction(tr("Filter Links"), this);
	filterLinksAct -> setEnabled(false);
	filterLinksAct->setStatusTip(tr("Filters Links of some weight out of the network"));
	filterLinksAct->setWhatsThis(tr("Filter Links\n\nFilters Link of some specific weight out of the network."));
	connect(filterLinksAct, SIGNAL(activated()), this, SLOT(slotFilterLinks()));

	changeBackColorAct = new QAction(QIcon(":/images/color.png"), tr("Change Background Color"), this);
	changeBackColorAct->setStatusTip(tr("Click to change the background color"));
	changeBackColorAct->setWhatsThis(tr("Background\n\nChanges background color"));
	connect(changeBackColorAct, SIGNAL(activated()), this, SLOT(slotBackgroundColor()));

	changeAllNodesColorAct = new QAction(QIcon(":/images/nodecolor.png"), tr("Change all Nodes Colors"),	this);
	changeAllNodesColorAct->setStatusTip(tr("Click to choose a new color for all nodes."));
	changeAllNodesColorAct->setWhatsThis(tr("All Nodes\n\nChanges all nodes color at once."));
	connect(changeAllNodesColorAct, SIGNAL(activated()), this, SLOT(slotAllNodesColor()) );

	changeAllNumbersColorAct = new QAction( tr("Change all Numbers Colors"),	this);
	changeAllNumbersColorAct->setStatusTip(tr("Click to change the color of all numbers."));
	changeAllNumbersColorAct->setWhatsThis(tr("Numbers\n\nChanges the color of all numbers."));
	connect(changeAllNumbersColorAct, SIGNAL(activated()), this, SLOT(slotAllNumbersColor()));

	changeAllLabelsColorAct = new QAction( tr("Change all Labels Colors"),	this);
	changeAllLabelsColorAct->setStatusTip(tr("Click to change the color of all node labels."));
	changeAllLabelsColorAct->setWhatsThis(tr("Numbers\n\nChanges the color of all node labels."));
	connect(changeAllLabelsColorAct, SIGNAL(activated()), this, SLOT(slotAllLabelsColor()));

	changeAllLinksColorAct = new QAction( tr("Change all Links Colors"), this);
	changeAllLinksColorAct->setStatusTip(tr("Click to change the color of all links."));
	changeAllLinksColorAct->setWhatsThis(tr("Background\n\nChanges all links color"));
	connect(changeAllLinksColorAct, SIGNAL(activated()), this, SLOT(slotAllLinksColor()));



	transformNodes2LinksAct = new QAction( tr("Transform Nodes to Links"),this);
	transformNodes2LinksAct->setStatusTip(tr("Transforms the network so that nodes become links and vice versa"));
	transformNodes2LinksAct->setWhatsThis(tr("Transform Nodes LinksAct\n\nTransforms network so that nodes become links and vice versa"));
	connect(transformNodes2LinksAct, SIGNAL(activated()), this, SLOT(slotTransformNodes2Links()));

	symmetrizeAct= new QAction(QIcon(":/images/symmetrize.png"), tr("Symmetrize Links"), this);
	symmetrizeAct->setShortcut(tr("Shift+R"));
	symmetrizeAct->setStatusTip(tr("Makes all edges reciprocal (thus, a symmetric graph)."));
	symmetrizeAct->setWhatsThis(tr("Symmetrize Edges\n\nTransforms all arcs to double links (edges). The result is a symmetric network"));
	connect(symmetrizeAct, SIGNAL(activated()), this, SLOT(slotSymmetrize()));	




	/**
	Layout menu actions
	*/
	strongColorationAct = new QAction ( tr("Strong Structural"), this);
	strongColorationAct -> setStatusTip( tr("Nodes are assigned the same color if they have identical in and out neighborhoods") );
	strongColorationAct -> setWhatsThis( tr("Click this to colorize nodes; Nodes are assigned the same color if they have identical in and out neighborhoods"));
	connect(strongColorationAct, SIGNAL(activated() ), this, SLOT(slotColorationStrongStructural()) );
	
	regularColorationAct = new QAction ( tr("Regular"), this);
	regularColorationAct -> setStatusTip( tr("Nodes are assigned the same color if they have neighborhoods of the same set of colors") );
	regularColorationAct -> setWhatsThis( tr("Click this to colorize nodes; Nodes are assigned the same color if they have neighborhoods of the same set of colors"));
	connect(regularColorationAct, SIGNAL(activated() ), this, SLOT(slotColorationRegular()) );
	
	randLayoutAct = new QAction( tr("Random"),	this);
	randLayoutAct ->setStatusTip(tr("Repositions the nodes in random places"));
	randLayoutAct->setWhatsThis(tr("Random Layout\n\n Repositions the nodes in random places"));
	connect(randLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutRandom()));

	randCircleLayoutAct = new QAction(tr("Random Circle"),	this);
	randCircleLayoutAct ->setStatusTip(tr("Repositions the nodes randomly on a circle"));
	randCircleLayoutAct->setWhatsThis(tr("Random Circle Layout\n\n Repositions the nodes randomly on a circle"));
	connect(randCircleLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutRandomCircle()));

	circleInDegreeLayoutAct = new QAction( tr("In-Degree"),	this);
	circleInDegreeLayoutAct ->setShortcut(tr("Ctrl+1"));
	circleInDegreeLayoutAct ->setStatusTip(tr("Repositions the nodes on circles of different radius. More In-Degree Central Nodes are positioned towards the centre."));
	circleInDegreeLayoutAct->setWhatsThis(tr("Circle In-Degree Centrality Layout\n\n Repositions the nodes on circles of different radius. More In-Degree Central Nodes are positioned towards the centre."));
	connect(circleInDegreeLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutCircleCentralityInDegree()));

	
	circleOutDegreeLayoutAct = new QAction( tr("Out-Degree"),	this);
	circleOutDegreeLayoutAct ->setShortcut(tr("Ctrl+2"));
	circleOutDegreeLayoutAct ->setStatusTip(tr("Repositions the nodes on circles of different radius. More Out-Degree Central Nodes are positioned towards the centre."));
	circleOutDegreeLayoutAct->setWhatsThis(tr("Circle Out-Degree Centrality Layout\n\n Repositions the nodes on circles of different radius. More Out-Degree Central Nodes are positioned towards the centre."));
	connect(circleOutDegreeLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutCircleCentralityOutDegree()));

	circleClosenessLayoutAct = new QAction( tr("Closeness"),	this);
	circleClosenessLayoutAct ->setShortcut(tr("Ctrl+3"));
	circleClosenessLayoutAct ->setStatusTip(tr("Repositions the nodes on circles of different radius. More Closeness Central Nodes are positioned towards the centre."));
	circleClosenessLayoutAct->setWhatsThis(tr("Circle Closeness Centrality Layout\n\n Repositions the nodes on circles of different radius. More Closeness Central Nodes are positioned towards the centre."));
	connect(circleClosenessLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutCircleCentralityCloseness()));

	circleBetweenessLayoutAct = new QAction( tr("Betweeness"), this);
	circleBetweenessLayoutAct ->setShortcut(tr("Ctrl+4"));
	circleBetweenessLayoutAct ->setStatusTip(tr("Repositions the nodes on circles of different radius. More Betweeness Central Nodes are positioned towards the centre."));
	circleBetweenessLayoutAct->setWhatsThis(tr("Circle Betweeness Centrality Layout\n\n Repositions the nodes on circles of different radius. More Betweeness Central Nodes are positioned towards the centre."));
	connect(circleBetweenessLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutCircleCentralityBetweeness()));

	circleInformationalLayoutAct = new QAction( tr("Informational"),	this);
	circleInformationalLayoutAct ->setEnabled(false);
	circleInformationalLayoutAct ->setShortcut(tr("Ctrl+5"));
	circleInformationalLayoutAct ->setStatusTip(tr("Repositions the nodes on circles of different radius. More Informational Central Nodes are situated towards the centre."));
	circleInformationalLayoutAct->setWhatsThis(tr("Circle Informational Centrality Layout\n\n Repositions the nodes on circles of different radius. More Informational Central Nodes are positioned towards the centre."));
	connect(circleInformationalLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutCircleCentralityInformational()));

	circleStressLayoutAct = new QAction( tr("Stress"),	this);
	circleStressLayoutAct ->setShortcut(tr("Ctrl+6"));
	circleStressLayoutAct ->setStatusTip(tr("Repositions the nodes on circles of different radius. More Stressed Central Nodes are positioned towards the centre."));
	circleStressLayoutAct->setWhatsThis(tr("Circle Stress Centrality Layout\n\n Repositions the nodes on circles of different radius. Nodes having greater Stress Centrality are situated towards the centre."));
	connect(circleStressLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutCircleCentralityStress() ) );
	
	circleGraphLayoutAct = new QAction( tr("Graph"),	this);
	circleGraphLayoutAct ->setShortcut(tr("Ctrl+7"));
	circleGraphLayoutAct ->setStatusTip(tr("Repositions the nodes on circles of different radius. More Graphed Central Nodes are positioned towards the centre."));
	circleGraphLayoutAct->setWhatsThis(tr("Circle Graph Centrality Layout\n\n Repositions the nodes on circles of different radius. Nodes having greater Graph Centrality are situated towards the centre."));
	connect(circleGraphLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutCircleCentralityGraph() ) );

	
	circleEccentrLayoutAct = new QAction( tr("Eccentricity"),	this);
	circleEccentrLayoutAct ->setShortcut(tr("Ctrl+8"));

	circleEccentrLayoutAct  ->setStatusTip(tr("Repositions the nodes on circles of different radius. Nodes of large eccentricity are positioned towards the centre."));
	circleEccentrLayoutAct ->setWhatsThis(tr("Circle Eccentricity Centrality Layout\n\n Repositions the nodes on circles of different radius. Nodes having greater Eccentricity Centrality are situated towards the centre."));
	connect(circleEccentrLayoutAct , SIGNAL(activated()), this, SLOT(slotLayoutCircleCentralityEccentr() ) );

	circleClearBackgrCirclesAct = new QAction( tr("Remove Layout GuideLines"), this);
	circleClearBackgrCirclesAct ->setStatusTip(tr("Removes Red GuideLines from the canvas."));
	circleClearBackgrCirclesAct->setWhatsThis(tr("Remove GuideLines\n\n Removes any guidelines (circles or horizontal lines) created for the network layout."));

	
	levelInDegreeLayoutAct = new QAction( tr("In-Degree"),this);
	levelInDegreeLayoutAct ->setShortcut(tr("Ctrl+Shift+1"));
	levelInDegreeLayoutAct ->setStatusTip(tr("Repositions the nodes on levels of different height. More In-Degree Central Nodes are situated on higher levels."));
	levelInDegreeLayoutAct->setWhatsThis(tr("Level In-Degree Centrality Layout\n\n Repositions the nodes on levels of different height. More In-Degree Central Nodes are situated on higher levels."));
	connect(levelInDegreeLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutLevelCentralityInDegree()));

	levelOutDegreeLayoutAct  = new QAction( tr("Out-Degree"),this);
	levelOutDegreeLayoutAct  ->setShortcut(tr("Ctrl+Shift+2"));
	levelOutDegreeLayoutAct  ->setStatusTip(tr("Repositions the nodes on levels of different height. More Out-Degree Central Nodes are situated on higher levels."));
	levelOutDegreeLayoutAct ->setWhatsThis(tr("Level Out-Degree Centrality Layout\n\n Repositions the nodes on levels of different height. More Out-Degree Central Nodes are situated on higher levels."));
	connect(levelOutDegreeLayoutAct , SIGNAL(activated()), this, SLOT(slotLayoutLevelCentralityOutDegree()));

	levelClosenessLayoutAct = new QAction( tr("Closeness"),	this);
	levelClosenessLayoutAct ->setShortcut(tr("Ctrl+Shift+3"));
	levelClosenessLayoutAct ->setStatusTip(tr("Repositions the nodes on levels of different height. More Closeness Central Nodes are situated on higher levels."));
	levelClosenessLayoutAct->setWhatsThis(tr("level Closeness Centrality Layout\n\n Repositions the nodes on levels of different height. More Closeness Central Nodes are situated on higher levels."));
	connect(levelClosenessLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutLevelCentralityCloseness()));

	levelBetweenessLayoutAct = new QAction( tr("Betweeness"),	this);
	levelBetweenessLayoutAct ->setShortcut(tr("Ctrl+Shift+4"));
	levelBetweenessLayoutAct ->setStatusTip(tr("Repositions the nodes on levels of different height. More Betweeness Central Nodes are situated on higher levels."));
	levelBetweenessLayoutAct->setWhatsThis(tr("level Betweeness Centrality Layout\n\n Repositions the nodes on levels of different height. More Betweeness Central Nodes are situated on higher levels."));
	connect(levelBetweenessLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutLevelCentralityBetweeness()));

	levelInformationalLayoutAct = new QAction( tr("Informational"),	this);
	levelInformationalLayoutAct ->setShortcut(tr("Ctrl+Shift+5"));
	levelInformationalLayoutAct -> setEnabled(false);
	levelInformationalLayoutAct ->setStatusTip(tr("Repositions the nodes on levels of different height. More Informational Central Nodes are situated on higher levels."));
	levelInformationalLayoutAct->setWhatsThis(tr("Level Informational Centrality Layout\n\n Repositions the nodes on levels of different height. More Informational Central Nodes are situated on higher levels."));
	connect(levelInformationalLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutLevelCentralityInformational()));

	springLayoutAct= new QAction(tr("Spring Embedder"), this);
	springLayoutAct->setShortcut(tr("Alt+1"));
	springLayoutAct->setStatusTip(tr("All nodes repel each other while the connected ones are attracted as if connected by springs."));
	springLayoutAct->setWhatsThis(tr("Spring Embedder Layout\n\n In this model, nodes are regarded as physical bodies (i.e. electrons) which exert repelling forces to each other, while edges are springs connecting adjacents nodes. Non-adjacent nodes repel each other while connected nodes are The algorithm continues until the system retains an equilibrium state in which all forces cancel each other. "));
	connect(springLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutSpringEmbedder()));

	FRLayoutAct= new QAction( tr("Fruchterman-Reingold"),	this);
	FRLayoutAct->setShortcut(tr("Alt+2"));
	FRLayoutAct->setEnabled(true);
	FRLayoutAct->setStatusTip(tr("Repelling forces between all nodes, and attracting forces between adjacent nodes."));
	FRLayoutAct->setWhatsThis(tr("Fruchterman-Reingold Layout\n\n Embeds a layout all nodes according to a model in which	repelling forces are used between every pair of nodes, while attracting forces are used only between adjacent nodes. The algorithm continues until the system retains its equilibrium state where all forces cancel each other."));
	connect(FRLayoutAct, SIGNAL(activated()), this, SLOT(slotLayoutFruchterman()));


	zoomInAct = new QAction(QIcon(":/images/zoomin.png"), tr("Zoom &in"),  this);
	zoomInAct->setShortcut(tr("Ctrl++"));
	zoomInAct->setToolTip(tr("Zoom in (Ctrl++)"));
	zoomInAct->setStatusTip(tr("Zooms inside the actual network."));
	zoomInAct->setWhatsThis(tr("Zoom In.\n\nZooms in. What else did you expect?"));


	zoomOutAct = new QAction(QIcon(":/images/zoomout.png"), tr("Zoom &out"),  this);
	zoomOutAct->setShortcut(tr("Ctrl+-"));
	zoomOutAct->setToolTip(tr("Zoom out (Ctrl+-)"));
	zoomOutAct->setStatusTip(tr("Zooms out of the actual network."));
	zoomOutAct->setWhatsThis(tr("Zoom out.\n\nZooms out. What else did you expect?"));

	nodeSizeProportionalOutDegreeAct= new QAction(tr("NodeSize = F (OutDegree)"), this);
	nodeSizeProportionalOutDegreeAct->setShortcut(tr("Alt+3"));
	nodeSizeProportionalOutDegreeAct->setStatusTip(tr("Resizes all nodes according to their out edges."));
	nodeSizeProportionalOutDegreeAct->setWhatsThis(tr("NodeSize = F (OutDegree) \n\n Adjusts the size of each node according to their out-edges (OutDegree). The more out-likned a node is, the bigger will appear..."));
	nodeSizeProportionalOutDegreeAct->setCheckable(true);
	nodeSizeProportionalOutDegreeAct->setChecked(false);
	connect(nodeSizeProportionalOutDegreeAct, SIGNAL(triggered(bool)), this, SLOT(slotLayoutNodeSizeProportionalOutEdges(bool)));


	nodeSizeProportionalInDegreeAct= new QAction(tr("NodeSize = F (InDegree)"), this);
	nodeSizeProportionalInDegreeAct->setShortcut(tr("Alt+4"));
	nodeSizeProportionalInDegreeAct->setStatusTip(tr("Resizes all nodes according to their in edges."));
	nodeSizeProportionalInDegreeAct->setWhatsThis(tr("NodeSize = F (InDegree) \n\n This method adjusts the size of each node according to their in-edges (InDegree). The more in-linked a node is, the bigger will appear..."));
	nodeSizeProportionalInDegreeAct->setCheckable(true);
	nodeSizeProportionalInDegreeAct->setChecked(false);
	connect(nodeSizeProportionalInDegreeAct, SIGNAL(triggered(bool)), this, SLOT(slotLayoutNodeSizeProportionalInEdges(bool)));



	/**
	Statistics menu actions
	*/

	symmetryAct = new QAction(QIcon(":/images/symmetry.png"), tr("Network Symmetry"), this);
	symmetryAct ->setShortcut(tr("Shift+S"));
	symmetryAct->setStatusTip(tr("Tests if the network is symmetric or not"));
	symmetryAct->setWhatsThis(tr("Network Symmetry\n\n A network is symmetric when all edges are reciprocal, or, in mathematical language, when the adjacency matrix is symmetric."));
	connect(symmetryAct, SIGNAL(activated()), this, SLOT(slotCheckSymmetry()));

 
	graphDistanceAct = new QAction(QIcon(":/images/distance.png"),  tr("Graph Distance"), this);
	graphDistanceAct ->setShortcut(tr("Ctrl+G"));
	graphDistanceAct->setStatusTip(tr("Calculates the length of the shortest path between two nodes..."));
	graphDistanceAct->setWhatsThis(tr("Graph Distance\n\n The graph distance (or geodesic distance) of two nodes is the length (number of edges) of the shortest path between them."));
	connect(graphDistanceAct, SIGNAL(activated()), this, SLOT(slotGraphDistance()));

	distanceMatrixAct = new QAction(QIcon(":/images/dm.png"), tr("Distance &Matrix"),this);
	distanceMatrixAct ->setShortcut(tr("Ctrl+M"));
	distanceMatrixAct->setStatusTip(tr("Displays the matrix of graph distances between all nodes"));
	distanceMatrixAct->setWhatsThis(tr("Distance Matrix\n\n A distance matrix is a NxN matrix, where the (i,j) element is the graph distance from node i to node j."));
	connect(distanceMatrixAct, SIGNAL(activated()), this, SLOT( slotViewDistanceMatrix() ) );
	
	diameterAct = new QAction(QIcon(":/images/diameter.png"), tr("Diameter"),this);
	diameterAct ->setShortcut(tr("Ctrl+D"));
	diameterAct->setStatusTip(tr("Calculates and displays the diameter of the network."));
	diameterAct->setWhatsThis(tr("Diameter\n\n The Diameter of a network is the maximum graph distance (maximum shortest path length) between any two nodes of the network."));
	connect(diameterAct, SIGNAL(activated()), this, SLOT(slotDiameter()));

	averGraphDistanceAct = new QAction(QIcon(":/images/avdistance.png"), tr("Average Graph Distance"),this);
	averGraphDistanceAct ->setShortcut(tr("Ctrl+B"));
	averGraphDistanceAct->setStatusTip(tr("Calculates and displays the average shortest path length."));
	averGraphDistanceAct->setWhatsThis(tr("Average Graph Distance\n\n This the average length of all shortest paths between the connected pair of nodes of the network."));
	connect(averGraphDistanceAct, SIGNAL(activated()), this, SLOT(slotAverageGraphDistance()));


	clusteringCoefAct = new QAction(QIcon(":/images/clique.png"), tr("Clustering Coefficient"),this);
	clusteringCoefAct ->setShortcut(tr("Ctrl+C"));
	clusteringCoefAct->setStatusTip(tr("Calculates and displays the average Clustering Coefficient of the network."));
	clusteringCoefAct->setWhatsThis(tr("Clustering Coefficient\n\n The Clustering Coefficient of a vertex quantifies how close the vertex and its neighbors are to being a clique. \n "));
	connect(clusteringCoefAct, SIGNAL(activated()), this, SLOT(slotClusteringCoefficient() )  );

	cOutDegreeAct = new QAction(tr("OutDegree"),	this);
	cOutDegreeAct->setStatusTip(tr("Calculates and displays OutDegree Centralities"));
	cOutDegreeAct->setWhatsThis(tr("OutDegree Centrality\n\n For each node k, this is the number of arcs starting from it. This is oftenly a measure of activity."));
	connect(cOutDegreeAct, SIGNAL(activated()), this, SLOT(slotCentralityOutDegree()));

	cInDegreeAct = new QAction(tr("InDegree"),	 this);
	cInDegreeAct->setStatusTip(tr("Calculates and displays InDegree Centralities"));
	cInDegreeAct->setWhatsThis(tr("InDegree Centrality\n\n For each node k, this the number of arcs ending at k. Most in-degree central node might be considered more prominent among others. "));
	connect(cInDegreeAct, SIGNAL(activated()), this, SLOT(slotCentralityInDegree()));

	cClosenessAct = new QAction(tr("Closeness"),	 this);
	cClosenessAct->setStatusTip(tr("Calculates and displays Closeness Centralities"));
	cClosenessAct->setWhatsThis(tr("Closeness Centrality\n\n For each node k, this the invert sum of the shortest distances between k and every other node. It is interpreted as the ability to access information through the \"grapevine\" of network members. "));
	connect(cClosenessAct, SIGNAL(activated()), this, SLOT(slotCentralityCloseness()));

	cBetweenessAct = new QAction(tr("Betweeness"),	 this);
	cBetweenessAct->setStatusTip(tr("Calculates and displays Betweeness Centralities"));
	cBetweenessAct->setWhatsThis(tr("Betweeness Centrality\n\n For each node k, this is the ratio of all geodesics between pairs of nodes which run through k. It reflects how often an node lies on the geodesics between the other nodes of the network. It can be interpreted as a measure of control."));
	connect(cBetweenessAct, SIGNAL(activated()), this, SLOT(slotCentralityBetweeness()));

	cGraphAct = new QAction(tr("Graph"),	this);
	cGraphAct->setStatusTip(tr("Calculates and displays Graph Centralities"));
	cGraphAct->setWhatsThis(tr("Graph Centrality\n\n For each node k, this is the invert of the maximum of all geodesic distances from k to all other nodes in the network. Nodes with high GC have short distances to all other nodes in the graph. "));
	connect(cGraphAct, SIGNAL(activated()), this, SLOT(slotCentralityGraph()));

	cStressAct = new QAction(tr("Stress"),	 this);
	cStressAct->setStatusTip(tr("Calculate and display Stress Centrality"));
	cStressAct->setWhatsThis(tr("Stress Centrality\n\n For each node k, this is the total number of geodesics between all other nodes which run through k. When one node falls on all other geodesics between all the remaining (N-1) nodes, then we have a star graph with maximum Stress Centrality"));
	connect(cStressAct, SIGNAL(activated()), this, SLOT(slotCentralityStress()));


	cEccentAct = new QAction(tr("Eccentricity"), this);
	cEccentAct ->setStatusTip(tr("Calculate and display Eccentricity Centrality"));
	cEccentAct ->setWhatsThis(tr("Stress Centrality\n\n For each node k, this is the largest geodesic distance (k,t) from every other vertex t. Therefore, EC(u) reflects how far, at most, is each node from every other node."));
	connect(cEccentAct , SIGNAL(activated()), this, SLOT(slotCentralityEccentricity()));

	cInformationalAct = new QAction(tr("Informational"),	this);
	cInformationalAct ->setEnabled(FALSE);
	cInformationalAct->setStatusTip(tr("Calculate and display Informational Centrality"));
	cInformationalAct->setWhatsThis(tr("Informational Centrality\n\n Calculate and display Informational Centrality"));
	connect(cInformationalAct, SIGNAL(activated()), this, SLOT(slotCentralityInformational()));

	
	/**
	Options menu actions
	*/
	displayNodeNumbersAct = new QAction( tr("Display Num&bers"), this );
	displayNodeNumbersAct->setStatusTip(tr("Toggles displaying of node numbers"));
	displayNodeNumbersAct->setWhatsThis(tr("Display Numbers\n\nEnables/disables node numbers"));
	displayNodeNumbersAct->setCheckable (true);
	displayNodeNumbersAct->setChecked(true);
	connect(displayNodeNumbersAct, SIGNAL(toggled(bool)), this, SLOT(slotDisplayNodeNumbers(bool)));
		
	displayNodeLabelsAct= new QAction(tr("Display Labels"),	this );
	displayNodeLabelsAct->setStatusTip(tr("Toggles displaying of node labels"));
	displayNodeLabelsAct->setWhatsThis(tr("Display Labels\n\nEnables/disables node labels"));
 	displayNodeLabelsAct->setCheckable (true);
	displayNodeLabelsAct->setChecked(false);
	connect(displayNodeLabelsAct, SIGNAL(toggled(bool)), this, SLOT(slotDisplayNodeLabels(bool)));

	displayLinksAct = new QAction(tr("Display Links"),	this);
	displayLinksAct->setStatusTip(tr("Toggle to display or not links"));
	displayLinksAct->setWhatsThis(tr("Display Links\n\nClick to enable or disable displaying of links"));
	displayLinksAct->setCheckable(true);
	displayLinksAct->setChecked(true);
	connect(displayLinksAct, SIGNAL(toggled(bool)), this, SLOT(slotToggleLinks(bool)) );

	displayLinksWeightNumbersAct = new QAction(tr("Display Weight Numbers"),	this);
	displayLinksWeightNumbersAct->setStatusTip(tr("Toggles displaying of numbers of links weights"));
	displayLinksWeightNumbersAct->setWhatsThis(tr("Display Weight Numbers\n\nClick to enable or disable displaying numbers of links weight"));
	displayLinksWeightNumbersAct->setCheckable(true);
	displayLinksWeightNumbersAct->setChecked(false);
	connect(displayLinksWeightNumbersAct, SIGNAL(toggled(bool)), this, SLOT(slotDisplayLinksWeightNumbers(bool)) );

	displayLinksArrowsAct = new QAction( tr("Display Arrows"),this);
	displayLinksArrowsAct->setStatusTip(tr("Toggles displaying of arrows in the end of links"));
	displayLinksArrowsAct->setWhatsThis(tr("Display Arrows\n\nClick to enable or disable displaying of arrows in the end of links"));
	displayLinksArrowsAct->setCheckable(true);
	displayLinksArrowsAct->setChecked(true);
	connect(displayLinksArrowsAct, SIGNAL(toggled(bool)), this, SLOT(slotDisplayLinksArrows(bool)) );

	drawLinksWeightsAct = new QAction( tr("Thickness=Weight"), this);
	drawLinksWeightsAct->setStatusTip(tr("Draws links as thick as their weights (if specified)"));
	drawLinksWeightsAct->setWhatsThis(tr("Draw As Thick As Weights\n\nClick to toggle having all links as thick as their weight (if specified)"));
	drawLinksWeightsAct->setCheckable(true);
	drawLinksWeightsAct->setChecked(false);
	drawLinksWeightsAct->setEnabled(false);
	connect(drawLinksWeightsAct, SIGNAL(toggled(bool)), this, SLOT(slotLinksThickWeights()) );

	drawLinksBezier = new QAction( tr("Bezier Curves"),	this);
	drawLinksBezier->setStatusTip(tr("Draws links as Bezier curves"));
	drawLinksBezier->setWhatsThis(tr("Links Bezier\n\nEnables/Disables drawing Links as Bezier curves."));
	drawLinksBezier->setCheckable(true);
	drawLinksBezier->setChecked (false);
	drawLinksBezier->setEnabled(false);
	connect(drawLinksBezier, SIGNAL(toggled(bool)), this, SLOT(slotLinksBezier(bool)) );

	
	/**
	Options > View menu actions
	*/

	antialiasingAct = new QAction(tr("Anti-Aliasing"), this);
	antialiasingAct ->setShortcut(tr("F8"));
	antialiasingAct ->setStatusTip(tr("Enables/disables anti-aliasing"));
	antialiasingAct ->setWhatsThis(tr("Enable or disable Anti-Aliasing\n\n Anti-aliasing is a technique which makes nodes, lines and text, smoother and fancier. But it comes at the cost of speed..."));
	antialiasingAct ->setCheckable(TRUE);
	antialiasingAct ->setChecked (true);
	connect(antialiasingAct , SIGNAL(toggled(bool)), this, SLOT(slotAntialiasing(bool)));


	showProgressBarAct = new QAction(tr("Progress Bars"), this);
	showProgressBarAct ->setShortcut(tr("F10"));
	showProgressBarAct->setStatusTip(tr("Enables/disables Progress Bars"));
	showProgressBarAct->setWhatsThis(tr("Enable or disable Progress Bars\n\nProgress Bars may appear during time-cost operations. Enabling progressBar has a significant cpu cost but lets you know about the progress of a given operation."));
	showProgressBarAct->setCheckable(true);
	showProgressBarAct->setChecked (false);
	connect(showProgressBarAct, SIGNAL(toggled(bool)), this, SLOT(slotShowProgressBar(bool)));

	printDebugAct = new QAction(tr("Debug Messages"),	this);
	printDebugAct ->setShortcut(tr("F9"));
	printDebugAct->setStatusTip(tr("Enables/disables printing debug messages to stdout"));
	printDebugAct->setWhatsThis(tr("Enables or disable Debug Messages\n\nPrinting debug messages to strerr. Enabling has a significant cpu cost but lets you know what SocNetV is actually doing."));
	printDebugAct->setCheckable(true);
	printDebugAct->setChecked (FALSE);
	printDebug=FALSE;
	connect(printDebugAct, SIGNAL(toggled(bool)), this, SLOT(slotPrintDebug(bool)));



	viewToolBar = new QAction(tr("Toolbar"), this);
	viewToolBar->setStatusTip(tr("Enables/disables the toolbar"));
	viewToolBar->setWhatsThis(tr("Enable or disable Toolbar\n\nThe toolbar is the widget right below the menu, and carries useful icons. You can disable it if you like..."));
	viewToolBar->setCheckable(true);
	viewToolBar->setChecked(true);
	connect(viewToolBar, SIGNAL(toggled(bool)), this, SLOT(slotViewToolBar(bool)));

	viewStatusBar = new QAction(tr("Statusbar"),	this);
	viewStatusBar->setStatusTip(tr("Enables/disables the statusbar"));
	viewStatusBar->setWhatsThis(tr("Enable or disable Statusbar\n\nThe statusbar is the widget at the bottom of the window, where messages appear. You might want to disable it..."));
	viewStatusBar->setCheckable(true);
	viewStatusBar->setChecked(true);
	connect(viewStatusBar, SIGNAL(toggled(bool)), this, SLOT(slotViewStatusBar(bool)));
	


	/**
	Help menu actions
	*/
	helpApp = new QAction(QIcon(":/images/help.png"), tr("Manual"),	this);
	helpApp ->setShortcut(tr("F1"));
	helpApp->setStatusTip(tr("Read the manual..."));
	helpApp->setWhatsThis(tr("Manual\n\nDisplays the documentation of SocNetV"));
	connect(helpApp, SIGNAL(activated()), this, SLOT(slotHelp()));

	tipsApp = new QAction(tr("Tip of the Day"), this);
	tipsApp->setStatusTip(tr("Read useful tips"));
	tipsApp->setWhatsThis(tr("Quick Tips\n\nDisplays some useful and quick tips"));
	connect(tipsApp, SIGNAL(activated()), this, SLOT(slotTips()));

	helpAboutApp = new QAction(tr("About SocNetV"), this);
	helpAboutApp->setStatusTip(tr("About SocNetV"));
	helpAboutApp->setWhatsThis(tr("About\n\nBasic information about SocNetV"));
	connect(helpAboutApp, SIGNAL(activated()), this, SLOT(slotHelpAbout()));



	helpAboutQt = new QAction(tr("About Qt"), this);
	helpAboutQt->setStatusTip(tr("About Qt"));
	helpAboutQt->setWhatsThis(tr("About\n\nAbout Qt"));
	connect(helpAboutQt, SIGNAL(activated()), this, SLOT(slotAboutQt() ) );
}



/**
  Creates and populates the MenuBar
*/
void MainWindow::initMenuBar() {


/** menuBar entry networkMenu */
	//networkMenu=new QMenu();
	networkMenu = menuBar()->addMenu(tr("&Network"));
	networkMenu -> addAction(fileNew);
	networkMenu  -> addAction(fileOpen);
	networkMenu ->addSeparator();
	networkMenu -> addAction (viewNetworkFileAct);
	networkMenu ->addSeparator();
	networkMenu -> addAction (viewSociomatrixAct);
	networkMenu ->addSeparator();
	randomNetworkMenu = new QMenu(tr("Create Random Network"));
	networkMenu ->addMenu (randomNetworkMenu);
	randomNetworkMenu -> addAction (createSmallWorldRandomNetworkAct);
	randomNetworkMenu -> addAction (createUniformRandomNetworkAct );
//  createConnectedRandomNetworkAct -> addTo(randomNetworkMenu);
// createGaussianRandomNetworkAct -> addTo(randomNetworkMenu);
	randomNetworkMenu -> addAction (createLatticeNetworkAct);
	randomNetworkMenu -> addAction (createSameDegreeRandomNetworkAct);
	networkMenu->addSeparator();

	
	
	networkMenu  -> addSeparator();
	networkMenu  -> addAction(fileSave);
	networkMenu  -> addAction(fileSaveAs);
	networkMenu  -> addSeparator();
	
	exportSubMenu = networkMenu  -> addMenu(tr("Export..."));

	
	exportSubMenu -> addAction (exportBMP);
	exportSubMenu -> addAction (exportPNG);
	exportSubMenu -> addAction (exportPDF);
	exportSubMenu -> addSeparator();
	exportSubMenu -> addAction (exportSM);
	exportSubMenu -> addAction (exportPajek);
	//   exportList->addTo(exportSubMenu);
	//   exportDL->addTo(exportSubMenu);
	//   exportGW->addTo(exportSubMenu);

	networkMenu  -> addSeparator();
	networkMenu  -> addAction(printNetwork);
	networkMenu  -> addSeparator();
	networkMenu  -> addAction(fileClose);
	networkMenu  -> addAction(fileQuit);



  
 /** menuBar entry editMenu */
	
	editMenu = menuBar()->addMenu(tr("&Edit"));
	editNodeMenu = new QMenu(tr("Node..."));
	editNodeMenu -> setIcon(QIcon(":/images/node.png"));
	editMenu -> addMenu ( editNodeMenu );
	editNodeMenu -> addAction (findNodeAct);
	editNodeMenu -> addAction (addNodeAct);
	editNodeMenu -> addAction (removeNodeAct);
	editNodeMenu -> addAction (changeNodeLabelAct);
	editNodeMenu -> addAction (changeNodeColorAct);
	editNodeMenu -> addAction (changeNodeSizeAct);
	editNodeMenu -> addAction (changeNodeValueAct);

	editNodeMenu -> addSeparator();
	editNodeMenu -> addAction (changeAllNodesSizeAct);
	editNodeMenu -> addAction (changeAllNodesShapeAct);
	editNodeMenu -> addAction (changeNumbersSizeAct);
	editNodeMenu -> addAction (changeLabelsSizeAct);
	
	editLinkMenu = new QMenu(tr("Link..."));
	editLinkMenu -> setIcon(QIcon(":/images/line.png"));
	editMenu-> addMenu (editLinkMenu);
	editLinkMenu -> addAction(addLinkAct);
	editLinkMenu -> addAction(removeLinkAct);
	editLinkMenu -> addAction(changeLinkLabelAct);
	editLinkMenu -> addAction(changeLinkColorAct);
	editLinkMenu -> addAction(changeLinkWeightAct);
	
	editMenu ->addSeparator();
	filterMenu = new QMenu ( tr("Filter..."));
	editMenu ->addMenu(filterMenu);
	
//   filterNodesAct -> addTo(filterMenu);
	filterMenu -> addAction(filterLinksAct);
	
	editMenu ->addSeparator();
//   transformNodes2LinksAct -> addTo (editMenu);
	editMenu -> addAction (symmetrizeAct);
	
	colorOptionsMenu=new QMenu(tr("Colors"));
	colorOptionsMenu -> setIcon(QIcon(":/images/colorize.png"));
	editMenu -> addMenu (colorOptionsMenu);
	colorOptionsMenu -> addAction (changeBackColorAct);
	colorOptionsMenu -> addAction (changeAllNodesColorAct);
	colorOptionsMenu -> addAction (changeAllLinksColorAct);
	colorOptionsMenu -> addAction (changeAllNumbersColorAct);
	colorOptionsMenu -> addAction (changeAllLabelsColorAct);


/** menuBar entry layoutMenu  */
	
	layoutMenu = menuBar()->addMenu(tr("&Layout"));
//   colorationMenu = new QPopupMenu();
//   layoutMenu -> insertItem (tr("Colorization"), colorationMenu);
//   strongColorationAct -> addTo(colorationMenu);
//   regularColorationAct-> addTo(colorationMenu);
//   layoutMenu->insertSeparator();
//   randomLayoutMenu = new QPopupMenu();
//   layoutMenu -> insertItem (tr("Random"), randomLayoutMenu );
//   randLayoutAct -> addTo (randomLayoutMenu);
//   randCircleLayoutAct -> addTo( randomLayoutMenu); 
	circleLayoutMenu = new QMenu(tr("In circles by centrality..."));
	circleLayoutMenu -> setIcon(QIcon(":/images/circular.png"));
	layoutMenu -> addMenu (circleLayoutMenu);
	circleLayoutMenu -> addAction (circleOutDegreeLayoutAct);
	circleLayoutMenu -> addAction (circleInDegreeLayoutAct);
	circleLayoutMenu -> addAction (circleClosenessLayoutAct);
	circleLayoutMenu -> addAction (circleBetweenessLayoutAct);
	circleLayoutMenu -> addAction (circleInformationalLayoutAct);
	circleLayoutMenu -> addAction (circleStressLayoutAct);
	circleLayoutMenu -> addAction (circleGraphLayoutAct);
	circleLayoutMenu -> addAction (circleEccentrLayoutAct);
	levelLayoutMenu = new QMenu (tr("In levels by centrality..."));
	levelLayoutMenu -> setIcon(QIcon(":/images/net3.png"));
	layoutMenu -> addMenu (levelLayoutMenu);
	levelLayoutMenu ->addAction ( levelInDegreeLayoutAct     );
	levelLayoutMenu ->addAction ( levelOutDegreeLayoutAct    );
	levelLayoutMenu ->addAction ( levelClosenessLayoutAct    );
	levelLayoutMenu ->addAction ( levelBetweenessLayoutAct   );
	levelLayoutMenu ->addAction ( levelInformationalLayoutAct);

	layoutMenu->addSeparator();
	physicalLayoutMenu = new QMenu (tr("Physical..."));
	layoutMenu -> addMenu (physicalLayoutMenu);
	physicalLayoutMenu -> addAction (springLayoutAct);
	physicalLayoutMenu -> addAction (FRLayoutAct);
	layoutMenu->addSeparator();
	layoutMenu->addAction(nodeSizeProportionalOutDegreeAct);
	layoutMenu->addAction(nodeSizeProportionalInDegreeAct);
	layoutMenu->addSeparator();
	layoutMenu -> addAction (circleClearBackgrCirclesAct);
//	layoutMenu->addSeparator();
//	layoutMenu->addAction(zoomInAct);
//	layoutMenu->addAction(zoomOutAct);
	


/** menuBar entry: statistics menu */
	statMenu = menuBar()->addMenu(tr("&Statistics"));
	statMenu -> addAction (symmetryAct);
//	statMenu -> addAction (netDensity);

	statMenu -> addSeparator();
	statMenu -> addAction (graphDistanceAct);
	statMenu -> addAction (averGraphDistanceAct);
	
	statMenu -> addAction (distanceMatrixAct);
	statMenu -> addAction (diameterAct);
	statMenu -> addSeparator();
	statMenu -> addAction (clusteringCoefAct);

	statMenu->addSeparator();
	centrlMenu = new QMenu(tr("Centralities"));
	statMenu->addMenu(centrlMenu); 
	centrlMenu -> addAction (cOutDegreeAct);
	centrlMenu -> addAction (cInDegreeAct);
	centrlMenu -> addAction (cClosenessAct);
	centrlMenu -> addAction (cBetweenessAct);
//   cInformationalAct -> addTo(centrlMenu);
	centrlMenu -> addAction (cGraphAct);
	centrlMenu -> addAction (cStressAct);
	centrlMenu -> addAction (cEccentAct);


/** menuBar entry optionsMenu  */
	optionsMenu = menuBar()->addMenu(tr("&Options"));
	nodeOptionsMenu=new QMenu(tr("Nodes"));
	nodeOptionsMenu -> setIcon(QIcon(":/images/nodes.png")); 

	optionsMenu -> addMenu (nodeOptionsMenu);
	nodeOptionsMenu -> addAction (displayNodeLabelsAct);
	nodeOptionsMenu -> addAction (displayNodeNumbersAct);

	linkOptionsMenu=new QMenu(tr("Links"));
	linkOptionsMenu -> setIcon(QIcon(":/images/line.png"));

	optionsMenu -> addMenu (linkOptionsMenu);
	linkOptionsMenu -> addAction (displayLinksAct);
	linkOptionsMenu -> addAction (displayLinksWeightNumbersAct);
	linkOptionsMenu -> addAction (displayLinksArrowsAct );
	linkOptionsMenu -> addSeparator();
	linkOptionsMenu -> addAction (drawLinksWeightsAct);
	linkOptionsMenu -> addAction (drawLinksBezier);
  
	viewOptionsMenu = new QMenu (tr("&View"));
	viewOptionsMenu -> setIcon(QIcon(":/images/view.png"));
	optionsMenu -> addMenu (viewOptionsMenu);
	viewOptionsMenu-> addAction (antialiasingAct);
	viewOptionsMenu-> addAction (printDebugAct);
	viewOptionsMenu-> addAction (showProgressBarAct);
	viewOptionsMenu-> addAction (viewToolBar);
	viewOptionsMenu-> addAction (viewStatusBar);
 

/**  menuBar entry helpMenu */
	helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu -> addAction (helpApp);
	helpMenu -> addAction (tipsApp);
	helpMenu -> addSeparator();
	helpMenu-> addAction (helpAboutApp);
	helpMenu-> addAction (helpAboutQt);


}



/**
	Initializes the toolbar
*/
void MainWindow::initToolBar(){
	toolBar = addToolBar("operations");
	toolBar -> addAction (fileNew);
	toolBar -> addAction (fileOpen);
	toolBar -> addAction (fileSave);
	toolBar -> addAction (printNetwork);
	toolBar -> addSeparator();
	
	toolBar -> addAction(zoomInAct);

	//Create zooming widget
	zoomCombo = new QComboBox;
	zoomCombo -> setFocusPolicy(Qt::NoFocus);
	QStringList scales;
   	scales << tr("25%") << tr("50%") << tr("75%") << tr("100%") << tr("125%") << tr("150%")<<tr("175%")  ;
   	zoomCombo->addItems(scales);
   	zoomCombo->setCurrentIndex(3);

	toolBar -> addWidget(zoomCombo);
	toolBar -> addAction(zoomOutAct);

	toolBar -> addSeparator();

	QLabel *labelRotateSpinBox= new QLabel;
	labelRotateSpinBox ->setText(tr("Rotation:"));

	
	rotateSpinBox = new QSpinBox;
	rotateSpinBox ->setRange(-360, 360);
	rotateSpinBox->setSingleStep(1);
	rotateSpinBox->setValue(0);

	QGroupBox *rotateGroup = new QGroupBox();
	QHBoxLayout *rotateGroupLayout = new QHBoxLayout(rotateGroup);
	rotateGroupLayout->addWidget(labelRotateSpinBox);
    	rotateGroupLayout->addWidget(rotateSpinBox);

	toolBar -> addWidget(rotateGroup);


	toolBar -> addSeparator();
	toolBar -> addAction ( QWhatsThis::createAction (this));


}







//Creates a dock widget for instant menu access
void MainWindow::initToolBox(){
	toolBox = new QTabWidget;
	toolBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Ignored));


	//create widgets for the buttons group/tab
	addNodeBt= new QPushButton(QIcon(":/images/add.png"),tr("&Add Node"));	
	addNodeBt->setFocusPolicy(Qt::NoFocus);	
	addNodeBt->setToolTip(tr("Add a new node to the network (Ctrl+A). \n\n Alternately, you can create a new node \nin a specific position by double-clicking \non that spot of the canvas."));
	removeNodeBt= new QPushButton(QIcon(":/images/remove.png"),tr("&Remove Node"));	
	removeNodeBt->setFocusPolicy(Qt::NoFocus);	
	removeNodeBt->setToolTip(tr("Remove a node from the network (Ctrl+Shift+A). \n\n Alternately, you can remove a node \nby right-clicking on it."));

	addLinkBt= new QPushButton(QIcon(":/images/connect.png"),tr("Add &Link"));	
	addLinkBt->setFocusPolicy(Qt::NoFocus);	
	addLinkBt->setToolTip(tr("Add a new link to the network (Ctrl+L).\n\n Alternately, you can create a new link between two \nnodes by middle-clicking on them consequetively."));

	removeLinkBt= new QPushButton(QIcon(":/images/disconnect.png"),tr("Remove Link"));	
	removeLinkBt->setFocusPolicy(Qt::NoFocus);	
	removeLinkBt->setToolTip(tr("Remove a link from the network  \n\n Alternately, you can remove a link \nby right-clicking on it."));

	//create a layout for these widgets
	QGridLayout *buttonsGrid = new QGridLayout;
	buttonsGrid -> addWidget(addNodeBt, 0,0);
	buttonsGrid -> addWidget(removeNodeBt, 0,1);
	buttonsGrid -> addWidget(addLinkBt,1,0);
	buttonsGrid -> addWidget(removeLinkBt,1,1);
	buttonsGrid -> setRowStretch(2,1);   //fix vertical stretch
	//create a box with a title & a frame. Inside, display the vertical layout of widgets
	QGroupBox *buttonsGroup= new QGroupBox(tr("Edit Network"));
	buttonsGrid->setSpacing(0);
	buttonsGrid->setMargin(10);
	buttonsGroup->setLayout(buttonsGrid);

 	toolBox->addTab(buttonsGroup, tr("Edit"));


	//create widgets for Properties/Statistics group/tab
	QLabel *labelNodesLCD = new QLabel;
	labelNodesLCD->setText(tr("Total Nodes"));
	QLabel *labelEdgesLCD = new QLabel;
	labelEdgesLCD->setText(tr("Total Links"));
	nodesLCD=new QLCDNumber(7);
	nodesLCD->setSegmentStyle(QLCDNumber::Flat);
	nodesLCD->setToolTip(tr("Counts how many nodes (vertices) exist in the whole network."));
	edgesLCD=new QLCDNumber(7);
	edgesLCD->setSegmentStyle(QLCDNumber::Flat);
	edgesLCD->setToolTip(tr("Counts how many links (in and out) exist in the whole network."));

	QLabel *labelDensityLCD = new QLabel;
	labelDensityLCD->setText(tr("Density"));
	densityLCD=new QLCDNumber(7);
	densityLCD->setSegmentStyle(QLCDNumber::Flat);
	densityLCD->setToolTip(tr("The density of a network is the ratio of existing links to all possible links (n(n-1)) between nodes."));

	QLabel * labelOutLinkedNodesLCD= new QLabel;
	labelOutLinkedNodesLCD -> setText (tr("OutLinked Nodes:"));
	outLinkedNodesLCD=new QLCDNumber(7);
	outLinkedNodesLCD->setSegmentStyle(QLCDNumber::Flat);
	outLinkedNodesLCD->setToolTip(tr("This the number of nodes with outLinks\n to another node. They may also have \ninLinks or reciprocal links. \nMeaningful on directed graphs."));
	QLabel * labelInLinkedNodesLCD = new QLabel;
	labelInLinkedNodesLCD -> setText (tr("InLinked Nodes:"));
	inLinkedNodesLCD=new QLCDNumber(7);
	inLinkedNodesLCD->setSegmentStyle(QLCDNumber::Flat);
	inLinkedNodesLCD->setToolTip(tr("This the number of nodes with inLinks \nfrom another node. These may also have \noutLinks or reciprocal links.\nMeaningful on directed graphs."));
	QLabel * labelReciprocalLinkedNodesLCD = new QLabel;
	labelReciprocalLinkedNodesLCD-> setText (tr("Reciprocal-Linked:"));
	reciprocalLinkedNodesLCD=new QLCDNumber(7);
	reciprocalLinkedNodesLCD->setSegmentStyle(QLCDNumber::Flat);
	reciprocalLinkedNodesLCD->setToolTip(tr("This the number of nodes with reciprocal links, \nnamely, both inLinks and outLinks to another node."));

	//create a grid layout
	QGridLayout *propertiesGrid = new QGridLayout();

	propertiesGrid -> setColumnMinimumWidth(0, 10);
	propertiesGrid -> setColumnMinimumWidth(1, 10);

	propertiesGrid -> addWidget(labelNodesLCD, 0,0);
	propertiesGrid -> addWidget(labelEdgesLCD, 0,1);
	propertiesGrid -> addWidget(nodesLCD,1,0);
	propertiesGrid -> addWidget(edgesLCD,1,1);

	propertiesGrid -> addWidget(labelDensityLCD, 2,0);
	propertiesGrid -> addWidget(densityLCD,2,1);

	propertiesGrid -> addWidget(labelOutLinkedNodesLCD,3,0);
	propertiesGrid -> addWidget(outLinkedNodesLCD,3,1);
	propertiesGrid -> addWidget(labelInLinkedNodesLCD,4,0);
	propertiesGrid -> addWidget(inLinkedNodesLCD,4,1);
	propertiesGrid -> addWidget(labelReciprocalLinkedNodesLCD, 5,0);
	propertiesGrid -> addWidget(reciprocalLinkedNodesLCD,5,1);



	QLabel *dummyLabel = new QLabel;
	dummyLabel-> setText (" ");
	QLabel *labelNode = new QLabel;
	labelNode-> setText (tr("Active Node"));

	QLabel *labelSelectedNodeLCD = new QLabel;
	labelSelectedNodeLCD -> setText (tr("Node Number:"));
	labelSelectedNodeLCD -> setToolTip (tr("This is the number of the last selected node."));

	selectedNodeLCD =new QLCDNumber(7);
	selectedNodeLCD ->setSegmentStyle(QLCDNumber::Flat);

	QLabel *labelInLinksLCD = new QLabel;
	labelInLinksLCD -> setText (tr("Node In-Degree:"));
	labelInLinksLCD -> setToolTip (tr("This is the number of edges ending at the node you clicked on."));
	inLinksLCD=new QLCDNumber(7);
	inLinksLCD -> setSegmentStyle(QLCDNumber::Flat);
	inLinksLCD -> setToolTip (tr("This is the number of edges ending at the node you clicked on."));
	QLabel *labelOutLinksLCD = new QLabel;
	labelOutLinksLCD -> setText (tr("Node Out-Degree:"));
	labelOutLinksLCD -> setToolTip (tr("This is the number of edges starting from the node you clicked on."));
	outLinksLCD=new QLCDNumber(7);
	outLinksLCD -> setSegmentStyle(QLCDNumber::Flat);
	outLinksLCD -> setToolTip (tr("This is the number of edges starting from the node you clicked on."));

	QLabel *labelClucofLCD  = new QLabel;
	labelClucofLCD -> setText (tr("Clustering Coef."));
	labelClucofLCD -> setToolTip (tr("The Clustering Coefficient quantifies how close the vertex and its neighbors are to being a clique."));
	clucofLCD = new QLCDNumber(7);
	clucofLCD -> setSegmentStyle(QLCDNumber::Flat);
	clucofLCD -> setToolTip (tr("The proportion of links between the vertices within the neighbourhood of the clicked vertex, divided by the number of links that could possibly exist between them."));
	
	
	propertiesGrid -> addWidget(dummyLabel, 6,0);
	propertiesGrid -> addWidget(labelNode, 7,0);
	propertiesGrid -> addWidget(labelSelectedNodeLCD , 8,0);
	propertiesGrid -> addWidget(selectedNodeLCD ,8,1);
	propertiesGrid -> addWidget(labelInLinksLCD, 9,0);
	propertiesGrid -> addWidget(inLinksLCD, 9,1);
	propertiesGrid -> addWidget(labelOutLinksLCD, 10,0);
	propertiesGrid -> addWidget(outLinksLCD,10,1);
	propertiesGrid -> addWidget(labelClucofLCD, 11,0);
	propertiesGrid -> addWidget(clucofLCD,11,1);
	propertiesGrid -> setRowStretch(12,1);   //fix stretch
	
	//create a box with title
	QGroupBox *networkPropertiesGroup = new QGroupBox(tr("Network"));
	networkPropertiesGroup -> setLayout (propertiesGrid);


 	toolBox->addTab( networkPropertiesGroup, tr("Statistics"));
	toolBox->setMinimumWidth(buttonsGroup->sizeHint().width());



	// create some more widgets for the final tab: "Layout"
	moveSpringEmbedderBx = new QCheckBox(tr("Spring Embedder") );
	moveSpringEmbedderBx->setToolTip(tr("Embeds a spring-gravitational model on the network, where \neach node is regarded as physical object reppeling all \nother nodes, while springs between connected nodes attact them. \nThe result is \nconstant movement. This is a very SLOW process on networks with N > 100!"));

	moveFruchtermanBx = new QCheckBox(tr("Fruchterman-Reingold") );
	moveFruchtermanBx ->setEnabled(false);
	moveFruchtermanBx->setToolTip(tr("In Fruchterman-Reingold model, the vertices behave as atomic particles or celestial bodies, exerting attractive and repulsive forces to each other. Again, only vertices that are neighbours attract each other but, unlike Spring Embedder, all vertices repel each other. "));

	moveKamandaBx= new QCheckBox(tr("Kamanda-Kwei") );
	moveKamandaBx->setEnabled(false);
	moveKamandaBx->setToolTip(tr("!"));


	nodeSizeProportional2OutDegreeBx = new QCheckBox(tr("NodeSize = F (OutDegree)") );
	nodeSizeProportional2OutDegreeBx ->setEnabled(true);
	nodeSizeProportional2OutDegreeBx->setToolTip(tr("If you enable this, all nodes will be resized so that their size reflect their out-degree (the amount of links from them). To put it simply, more out-linked nodes will be bigger..."));

	nodeSizeProportional2InDegreeBx = new QCheckBox(tr("NodeSize = F (InDegree)") );
	nodeSizeProportional2InDegreeBx ->setEnabled(true);
	nodeSizeProportional2InDegreeBx->setToolTip(tr("If you enable this, all nodes will be resized so that their size reflect their in-degree (the amount of links to them from other nodes). To put it simply, more in-linked nodes will be bigger..."));

	QGridLayout *layoutGroupLayout = new QGridLayout();
    layoutGroupLayout -> addWidget(moveSpringEmbedderBx, 0,0);
    layoutGroupLayout -> addWidget(moveKamandaBx, 1,0);
	layoutGroupLayout -> addWidget(nodeSizeProportional2OutDegreeBx, 2,0);
	layoutGroupLayout -> addWidget(nodeSizeProportional2InDegreeBx, 3,0);

	layoutGroupLayout -> setRowStretch(4,1);   //fix stretch	

	//create a box with title
	QGroupBox *layoutGroup= new QGroupBox(tr("Layout"));
	layoutGroup->setLayout (layoutGroupLayout );
 	toolBox->addTab(layoutGroup, tr("Layout"));


	connect(moveSpringEmbedderBx, SIGNAL(stateChanged(int)),this, SLOT(layoutSpringEmbedder(int)));
	connect(moveFruchtermanBx, SIGNAL(stateChanged(int)),this, SLOT(layoutFruchterman(int)));

	connect(nodeSizeProportional2OutDegreeBx , SIGNAL(clicked(bool)),this, SLOT(slotLayoutNodeSizeProportionalOutEdges(bool)));
	connect(nodeSizeProportional2InDegreeBx , SIGNAL(clicked(bool)),this, SLOT(slotLayoutNodeSizeProportionalInEdges(bool)));

}









//FIXME this is a bug: Graph calls GraphicsWidget which calls this to call Graph!
void MainWindow::updateNodeCoords(int nodeNumber, int x, int y){
//	qDebug("MW: updateNodeCoords() for %i with x %i and y %i", nodeNumber, x, y);
	activeGraph.updateVertCoords(nodeNumber, x, y);
}




/**
	Initializes the status bar
*/
void MainWindow::initStatusBar() {
	statusBarDuration=2000;
	statusBar()->showMessage(tr("Ready."), statusBarDuration);
}







/**
	Initializes the scene and its graphicsWidget, the main widget of SocNetV
*/
void MainWindow::initView() {
	qDebug ("MW initView()");
	//create a scene
	scene=new QGraphicsScene();

	//create a view widget for this scene
	graphicsWidget=new GraphicsWidget(scene, this);
	//QGraphicsView can cache pre-rendered content in a QPixmap, which is then drawn onto the viewport. 
	graphicsWidget->setCacheMode(QGraphicsView::CacheNone);  //CacheBackground | CacheNone

 	graphicsWidget->setRenderHint(QPainter::Antialiasing, true);
	graphicsWidget->setRenderHint(QPainter::TextAntialiasing, true);
	graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, false);
	//Optimization flags:
	// By enabling the flag below, QGraphicsView will completely disable its implicit clipping
	graphicsWidget->setOptimizationFlag(QGraphicsView::DontClipPainter, false); 
	//
	graphicsWidget->setOptimizationFlag(QGraphicsView::DontSavePainterState, false);
	//Disables QGraphicsView's antialiasing auto-adjustment of exposed areas.
	graphicsWidget->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, false);
	//"QGraphicsScene applies an indexing algorithm to the scene, to speed up item discovery functions like items() and itemAt(). Indexing is most efficient for static scenes (i.e., where items don't move around). For dynamic scenes, or scenes with many animated items, the index bookkeeping can outweight the fast lookup speeds." So...
	scene->setItemIndexMethod(QGraphicsScene::NoIndex); //NoIndex | BspTreeIndex

 	graphicsWidget->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
 	graphicsWidget->setResizeAnchor(QGraphicsView::AnchorViewCenter);
	graphicsWidget->setFocusPolicy(Qt::StrongFocus);	

	printDebugAct->setChecked (FALSE);

	this->resize(900,700);

	//set minimum size of canvas
 	graphicsWidget->setMinimumSize( (qreal)  ( this->width()-toolBox->sizeHint().width() -30) , (qreal) ( this->height()-statusBar()->sizeHint().height() -toolBar->sizeHint().height() -menuBar()->sizeHint().height() -20 ) );
	qDebug ("MW initView(): now window size %i, %i, graphicsWidget size %i, %i, scene %f,%f",this->width(),this->height(), graphicsWidget->width(),graphicsWidget->height(), graphicsWidget->scene()->width(), graphicsWidget->scene()->height());
//	printDebugAct->setChecked (FALSE);
}




/**
	Resizes the scene when the window is resized.
*/
void MainWindow::resizeEvent( QResizeEvent * ){
	qDebug ("MW resizeEvent():INITIAL window size %i, %i, graphicsWidget size %i, %i, scene %f,%f",this->width(),this->height(), graphicsWidget->width(),graphicsWidget->height(), graphicsWidget->scene()->width(), graphicsWidget->scene()->height());

	//the area of the scene displayed by the CanvasView
	scene->setSceneRect(0, 0, (qreal) ( graphicsWidget->width() ), (qreal) (graphicsWidget->height() ) );
	qDebug ("MW resizeEvent(): now window size %i, %i, graphicsWidget size %i, %i, scene %f,%f",this->width(),this->height(), graphicsWidget->width(),graphicsWidget->height(), graphicsWidget->scene()->width(), graphicsWidget->scene()->height());
}


/**
	Initializes the default network parameters. 
	Also used when erasing a network to start a new one
*/
void MainWindow::initNet(){
	qDebug("MW: initNet() START INITIALISATION");
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	initNodeSize=4;
	initNodeColor="gold";
	initLinkColor="grey"; //"black";
	initLabelColor="black";
	initNumberColor="black";
 	initNumberSize=8; 
	initNodeShape="circle";
	initBackgroundColor="white"; //"gainsboro";

	minDuration=3000; //dialogue duration - obsolete
	maxNodes=5000;		//Max nodes used by createRandomNetwork dialogues
	labelDistance=8;
	numberDistance=5;
	totalLinks=0;
	networkName="";
	pajekFileLoaded=FALSE;
	adjacencyFileLoaded=FALSE;

	dotFileLoaded=FALSE;
	fileLoaded=FALSE;
	fileSaved=FALSE;
	networkModified=FALSE;
	fileSave->setIcon(QIcon(":/images/saved.png"));
	fileSave->setEnabled(false);

	fileContainsNodesCoords=false;
	fileContainsNodeColors=false;
	fileContainsLinksColors=false;

	markedNodeExists=FALSE;

	cursorPosGW=QPointF(-1,-1);
	clickedJimNumber=-1;
	linkClicked=FALSE;
	nodeClicked=FALSE;
	
	/** Clear previous network data */
	activeGraph.clear();
	
	activeGraph.setShowLabels(this->showLabels());
	activeGraph.setInitVertexColor(initNodeColor);
	activeGraph.setInitEdgeColor(initLinkColor);
	activeGraph.setInitVertexLabelColor(initLabelColor);
	activeGraph.setInitVertexShape(initNodeShape);
	activeGraph.setInitVertexSize(initNodeSize);

	/** Clear scene **/
	graphicsWidget->clear();
	

	//Clear LCDs
	nodesLCD->display(activeGraph.vertices());
	edgesLCD->display(activeGraph.totalEdges());
	densityLCD->display(activeGraph.density());
	inLinksLCD->display(0);
	outLinksLCD->display(0);
	clucofLCD->display(0);
	selectedNodeLCD->display(0);
	inLinkedNodesLCD -> display(activeGraph.verticesWithInEdges());
	outLinkedNodesLCD-> display(activeGraph.verticesWithOutEdges());
	reciprocalLinkedNodesLCD->display(activeGraph.verticesWithReciprocalEdges());

	nodeSizeProportional2OutDegreeBx->setChecked(false);
	nodeSizeProportional2InDegreeBx->setChecked(false);
  
	displayLinksWeightNumbersAct->setChecked(false);
	//set window title
	setWindowTitle(tr("Social Network Visualiser ")+VERSION);
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(tr("Ready"), statusBarDuration);
	qDebug("MW: initNet() INITIALISATION END");
}





/**
	Closes the application. 
	Asks to write any unsaved network data.
*/
void MainWindow::closeEvent( QCloseEvent* ce ) {
	if ( !networkModified )       {
		ce->accept();
		return;
	}
	switch( QMessageBox::information( this, "Save file",
				      tr("Do you want to save the changes") +
				      tr(" to the network file?"),
				      tr("Yes"), tr("No"), tr("Cancel"),
				      0, 1 ) )
	{
		case 0:
			slotFileSave();
			ce->accept();
			break;
		case 1:
			ce->accept();
			break;
		case 2:
		default: // just for sanity
			ce->ignore();
			break;
	}
}




/**
	Creates a new network
*/
void MainWindow::slotCreateNew() {
	slotFileClose();  
}




/**
	Prompts the user a directory dialogue to choose a file from.
	Loads the specified file, calling loadNetworkFile()
*/
void MainWindow::slotChooseFile() {
	qDebug("slotChooseFile()");
	bool m_fileLoaded=fileLoaded;
	QString m_fileName=fileName;
	statusBar()->showMessage(tr("Choose a network file..."));
	fileName = QFileDialog::getOpenFileName(this, tr("Select one file to open"), "", tr("All (*);;GraphML (*.graphml *.gml);;GraphViz (*.dot);;Adjacency (*.txt *.csv *.net);;Pajek (*.net *.pajek);;DL (*.dl *.net)"));
	
	if (!fileName.isEmpty()) {
		fileNameNoPath=fileName.split ("/" );
		if ( loadNetworkFile ( fileName ) == 1 ) {
			setWindowTitle("SocNetV "+ VERSION +" - "+fileNameNoPath.last());
			QString message=tr("Loaded network: ")+fileNameNoPath.last();
			statusBar()->showMessage(message, statusBarDuration);
		}
		else
			statusBar()->showMessage(tr("Error loading requested file. Aborted."), statusBarDuration);
	}
	else  {
		statusBar()->showMessage(tr("Opening aborted"), statusBarDuration);	
		//in case a file was opened
		if (m_fileLoaded) { fileLoaded=m_fileLoaded;  fileName=m_fileName; }
  	}

	qDebug()<<"FILENAME IS NOW:" <<fileName.toAscii();
}



/**
	Saves the network in the same file
*/
void MainWindow::slotFileSave() {
	statusBar()->showMessage(tr("Saving file..."));
	if (!fileLoaded && !networkModified ) {
		statusBar()->showMessage( QString(tr("No network loaded.")), statusBarDuration );
		return;
	}
	if ( fileName.isEmpty() )    {
		slotFileSaveAs();
		return;
	}
	fileNameNoPath=fileName.split ("/");
	if (pajekFileLoaded) {
		if ( slotExportPajek() )  fileSaved=TRUE;  
		else  fileSaved=FALSE; 
	}
	else if (adjacencyFileLoaded){
		if (slotExportSM() ) fileSaved=TRUE; 
		else  fileSaved=FALSE; 
	}
	else 
	switch( QMessageBox::information( this, "File Format-",
				      tr("Do you want to save this network ")+
				      tr("in Pajek-formatted or SocioMatrix - formatted file?"),
				      tr("Pajek"), tr("Sociomatrix"), tr("Cancel"),
				      0, 1 ) )
	{
		case 0:
			if ( slotExportPajek() )  fileSaved=TRUE; 
			else  fileSaved=FALSE; 
			break;
		case 1:
			if (slotExportSM() ) fileSaved=TRUE;
			else  fileSaved=FALSE; 
			break;
	}
	if (fileSaved)	{
		fileSave->setIcon(QIcon(":/images/saved.png"));
		fileSave->setEnabled(false);
		fileLoaded=TRUE; networkModified=FALSE;
	}
	else 
		 graphChanged();

	setWindowTitle( fileNameNoPath.last() );
	statusBar()->showMessage(tr("Network saved under filename: ")+fileNameNoPath.last()+tr("."), statusBarDuration );
}



/**
	Saves the network in a new file
*/
void MainWindow::slotFileSaveAs() {
	statusBar()->showMessage(tr("Saving network under new filename..."));
	QString fn = QFileDialog::getSaveFileName(this, 0, 0);
	if (!fn.isEmpty())  {
		fileName=fn;
		adjacencyFileLoaded=FALSE;
		pajekFileLoaded=FALSE;
		
		slotFileSave();
	}
	else  
		statusBar()->showMessage(tr("Saving aborted"), statusBarDuration);
	statusBar()->showMessage(tr("Ready."));
}




/**
	Closes the network. Saves it if necessary. 
	Used by createNew.
*/
void MainWindow::slotFileClose() {
	statusBar()->showMessage(tr("Closing file..."));
	if (networkModified) {
		switch ( QMessageBox::information (this, "Closing Network...",tr("Network has not been saved. \nDo you want to save before closing it?"), "Yes", "No",0,1))
		{
			case 0: slotFileSave(); break;
			case 1: break;
		}
	}
	statusBar()->showMessage(tr("Erasing old network data...."), statusBarDuration);
	initNet();	
	statusBar()->showMessage(tr("Ready."));
}



/**
	Prints whatever is viewable on the Graphics widget
*/
void MainWindow::slotPrintView() {
	statusBar()->showMessage(tr("Printing..."));
	QPrintDialog dialog(printer, this);
	if ( dialog.exec() )   {
 		QPainter painter(printer);
//		 QRect viewport = view.viewport()->rect();
 		graphicsWidget->render(&painter);
	};
	statusBar()->showMessage(tr("Ready."));
}




/**
	inits everything to defaults.
	Then calls loadFile function of activeGraph to load the network...
*/
int MainWindow::loadNetworkFile(QString fileName){
	qDebug("MW: loadNetworkFile");
	initNet(); 
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	qDebug("MW: calling activeGraph loadFile gw height %i", graphicsWidget->height() ) ;
	int loadFileStatus=activeGraph.loadFile(fileName, initNodeSize, initNodeColor, initLinkColor, initNodeShape, displayNodeLabelsAct->isChecked(), graphicsWidget->width(), graphicsWidget->height() );
	qDebug("MW: OK activeGraph.loadFile()  has finished. You should be seeing nodes by now! ");
	QApplication::restoreOverrideCursor();
	return loadFileStatus;
}


/**
*	This method is called from Graph when a network file is loaded.
*	It informs the MW about the type of the network so that it can display the appropiate message.
*/
void MainWindow::fileType(int type, QString networkName, int aNodes, int totalLinks){
	qDebug("MW: fileType()");
	switch( type ) 	{
		case 0:
			pajekFileLoaded=FALSE;
			adjacencyFileLoaded=FALSE;
			fileLoaded=FALSE;
			break;
		case 1:
			pajekFileLoaded=TRUE;
			adjacencyFileLoaded=FALSE;
			fileLoaded=TRUE;
			networkModified=FALSE;
// 			QMessageBox::information (this, "File Loaded", "Pajek formatted network\n"
// 			"\nNamed: "+QString(networkName)
// 			+"\nNodes: "+QString::number(aNodes)
// 			+"\nLinks: "+QString::number(totalLinks)+ ".", "OK",0);
			statusBar()->showMessage( QString(tr("Pajek formatted network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ), statusBarDuration);
			break;
		case 2:
			pajekFileLoaded=FALSE;
			adjacencyFileLoaded=TRUE;
			fileLoaded=TRUE;
			networkModified=FALSE;
			statusBar()->showMessage( QString(tr("Adjacency formatted network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ), statusBarDuration);
			break;
		case 3:
			pajekFileLoaded=FALSE;
			adjacencyFileLoaded=FALSE;
			dotFileLoaded=TRUE;
			fileLoaded=TRUE;
			networkModified=FALSE;
			statusBar()->showMessage( QString(tr("Dot formatted network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ), statusBarDuration);
			break;
		case 4:
			pajekFileLoaded=FALSE;
			adjacencyFileLoaded=FALSE;
			dotFileLoaded=FALSE;
			fileLoaded=TRUE;
			networkModified=FALSE;
			statusBar()->showMessage( QString(tr("GraphML formatted network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ), statusBarDuration);
			break;

		case 5:
			pajekFileLoaded=FALSE;
			adjacencyFileLoaded=FALSE;
			dotFileLoaded=FALSE;
			fileLoaded=TRUE;
			networkModified=FALSE;
			statusBar()->showMessage( QString(tr("DL-formatted network, named %1, loaded with %2 Nodes and %3 total Links.")).arg( networkName ).arg( aNodes ).arg(totalLinks ), statusBarDuration);
			break;
		default: // just for sanity
			pajekFileLoaded=FALSE;
			adjacencyFileLoaded=FALSE;
			fileLoaded=FALSE;
			QMessageBox::critical(this, "Error","Unrecognized format. \nPlease specify"
			" which is the file-format using Import Menu.","OK",0);
			break;
	}
	fileSave->setIcon(QIcon(":/images/saved.png"));
	fileSave->setEnabled(false);
}








/**
	Calls Graph::createVertex method to add a new RANDOM node into the activeGraph.
	Called when "Create Node" button is clicked on the Main Window.
*/
void MainWindow::addNode() {
	qDebug("MW: addNode(). Calling activeGraph::createVertex() for a new vertex (named -1)");
	activeGraph.createVertex(-1, graphicsWidget->width()-10,  graphicsWidget->height()-10);  // minus a  screen edge offset...
	qDebug("MW: addNode(). Calling activeGraph::createVertex() max width and height %i, %i", graphicsWidget->width()-10,  graphicsWidget->height()-10);
	statusBar()->showMessage(tr("New node (numbered %1) added.").arg(activeGraph.lastVertexNumber()) ,statusBarDuration);
}


/**
	Calls Graph::createVertex method to add a new node into the activeGraph.
	Called on double clicking
*/
void MainWindow::addNodeWithMouse(int num, QPointF p) {
	qDebug("MW: addNodeWithMouse(). Calling activeGraph::createVertex() for a vertice named %i", num);
	activeGraph.createVertex(num, p);
	statusBar()->showMessage(tr("New node (numbered %1) added.").arg(activeGraph.lastVertexNumber()) ,statusBarDuration);
}





/**
	Exports the network to a PNG image 
	Mediocre Quality but smaller file
*/

bool MainWindow::slotExportPNG(){
	qDebug("slotExportPNG");
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("The canvas is empty!\nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("Cannot export PNG.") ,statusBarDuration);
		return false;
	}
	QString fn = QFileDialog::getSaveFileName(this,tr("Save"), 0, tr("Image Files (*.png)"));
	if (fn.isEmpty())  {
		statusBar()->showMessage(tr("Saving aborted"), statusBarDuration);
		return false;
	}	
	tempFileNameNoPath=fn.split ("/");
	qDebug("slotExportPNG: grabing canvas");
	QPixmap picture;
	picture=QPixmap::grabWidget(graphicsWidget, graphicsWidget->rect());
	qDebug("slotExportPNG: adding logo");
	QPainter p;
	p.begin(&picture);
		p.setFont(QFont ("Helvetica", 10, QFont::Normal, FALSE));
		p.drawText(5,10,"SocNetV: "+tempFileNameNoPath.last());
	p.end();
	qDebug("slotExportPNG: checking filename");
	if (fn.contains("png", Qt::CaseInsensitive) ) {
		picture.toImage().save(fn, "PNG");
		QMessageBox::information(this, "Export to PNG...",tr("Image Saved as: ")+tempFileNameNoPath.last(), "OK",0);
 	}
	else {
		picture.toImage().save(fn+".png", "PNG");
		QMessageBox::information(this, "Export to PNG...",tr("Image Saved as: ")+tempFileNameNoPath.last()+".png" , "OK",0);
	}
	
	statusBar()->showMessage( tr("Exporting completed"), statusBarDuration );
	
	return true;   
}



/**
	Exports the network to a BMP image  
	Better Quality but larger file
*/
bool MainWindow::slotExportBMP(){
	qDebug(	"slotExportBMP()");
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("Nothing to export! \nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("Cannot export BMP.") ,statusBarDuration);
		return false;
	}
	QString format="bmp";
	QString fn = QFileDialog::getSaveFileName(this,tr("Save Image as"), 0,tr("Image Files (*.bmp)"));
	if (fn.isEmpty())  {
		statusBar()->showMessage(tr("Saving aborted"), statusBarDuration);
		return false;
	}	
	tempFileNameNoPath=fn.split ("/");

	QPixmap picture;
	qDebug("slotExportBMP: grabing canvas");
	picture=QPixmap::grabWidget(graphicsWidget, graphicsWidget->viewport()->rect());
	QPainter p;
	qDebug("slotExportBMP: adding logo");
	p.begin(&picture);
		p.setFont(QFont ("Helvetica", 10, QFont::Normal, FALSE));
		p.drawText(5,10,"SocNetV: "+tempFileNameNoPath.last());
	p.end();
	qDebug("slotExportBMP: checking file");
	if (fn.contains(format, Qt::CaseInsensitive) ) {
		picture.toImage().save(fn, format.toAscii());
		QMessageBox::information(this, tr("Export to BMP..."),tr("Image Saved as: ")+tempFileNameNoPath.last(), "OK",0);
 	}
	else {
		picture.toImage().save(fn+"."+format, format.toAscii());
		QMessageBox::information(this, tr("Export to BMP..."),tr("Image Saved as: ")+tempFileNameNoPath.last()+"."+format , "OK",0);
	}
	qDebug()<< "Exporting BMP to "<< fn;

	statusBar()->showMessage( tr("Exporting completed"), statusBarDuration );
	qDebug("Export finished!");
	return true;   
}






/**
	Exports the network to a PDF Document 
	Best Quality 
*/
bool MainWindow::slotExportPDF(){
	qDebug(	"slotExportPDF()");
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("The canvas is empty!\nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("Cannot export PDF.") ,statusBarDuration);
		return false;
	}
	
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export to PDF"), 0, tr("Portable Document Format files (*.pdf)"));
	if (fileName.isEmpty())  {
		statusBar()->showMessage(tr("Saving aborted"), statusBarDuration);
		return false;
	}	
    else {
		if (QFileInfo(fileName).suffix().isEmpty())
			fileName.append(".pdf");

		QPrinter printer(QPrinter::HighResolution);
		printer.setOutputFormat(QPrinter::PdfFormat);
		printer.setOutputFileName(fileName);
		QPainter painter(&printer);
		graphicsWidget->render(&painter);
	}
	qDebug()<< "Exporting PDF to "<< fileName;	
	tempFileNameNoPath=fileName.split ("/");
	QMessageBox::information(this, tr("Export to PDF..."),tr("File saved as: ")+tempFileNameNoPath.last() , "OK",0);
	statusBar()->showMessage( tr("Exporting completed"), statusBarDuration );
	return true;
}




/**
	FIXME: Import/Export belongs to Graph class

	Exports the network to a Pajek-formatted file
	Preserves node properties (positions, colours, etc)
*/
bool MainWindow::slotExportPajek(){
	qDebug ("MW: slotExportPajek");
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("Nothing to export! \nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("Cannot export to Pajek.") ,statusBarDuration);
		return false;
	}

	int weight=0;
	if (fileName.isEmpty()) {
		statusBar()->showMessage(tr("Saving network under new filename..."));
  		QString fn = QFileDialog::getSaveFileName(this,tr("Save"), 0,0);
 	 	if (!fn.isEmpty())  {
			fileName=fn;
  		}
  		else  {
   		 	statusBar()->showMessage(tr("Saving aborted"), statusBarDuration);
			return false;
 		}
	}

	QFile f( fileName );
	if ( !f.open( QIODevice::WriteOnly ) )  {
		statusBar()->showMessage( QString(tr("Could not write to %1")).arg(fileName), statusBarDuration );
		return false;
	}
	QTextStream t( &f );
   	t<<"*Network "<<networkName<<"\n";
	
   	t<<"*Vertices "<< activeNodes() <<"\n";
	QList<Vertex*>::iterator it;
	QList<Vertex*>::iterator jt;
	for (it=activeGraph.m_graph.begin(); it!=activeGraph.m_graph.end(); it++){ 
		qDebug()<<" Name x "<<  (*it)->name()  ;
		t<<(*it)->name()  <<" "<<"\""<<(*it)->label()<<"\"" ;
		t << " ic ";
		t<<  (*it)->color() ;
		qDebug()<<" Coordinates x " << (*it)->x()<< " "<<scene->width()<<" y " << (*it)->y()<< " "<<scene->height();
		t << "\t\t" <<(*it)->x()/(scene->width())<<" \t"<<(*it)->y()/(scene->height());
		t << "\t"<<(*it)->shape();
		t<<"\n";
	}

	t<<"*Arcs \n";
	qDebug("MW: Arcs");
	for (it=activeGraph.m_graph.begin(); it!=activeGraph.m_graph.end(); it++){ 
		for (jt=activeGraph.m_graph.begin(); jt!=activeGraph.m_graph.end(); jt++){ 
			qDebug("MW:  it=%i, jt=%i", (*it)->name(), (*jt)->name() );
			if  ( (weight=activeGraph.hasEdge( (*it)->name(), (*jt)->name())) !=0   &&   (activeGraph.hasEdge((*jt)->name(), (*it)->name())) == 0  )  {
				qDebug()<<"MW: slotExportPajek weight "<< weight << " color "<<  (*it)->outLinkColor( (*jt)->name() ) ;
			        t << (*it)->name() <<" "<<(*jt)->name()<< " "<<weight;
				//FIXME bug in outLinkColor() when we remove then add many nodes from the end
				t<< " c "<< (*it)->outLinkColor( (*jt)->name() );
          			t <<"\n";
			}

		}
	}
	
	t<<"*Edges \n";
	qDebug("MW: Edges");
	for (it=activeGraph.m_graph.begin(); it!=activeGraph.m_graph.end(); it++){ 
		for (jt=activeGraph.m_graph.begin(); jt!=activeGraph.m_graph.end(); jt++){ 
			qDebug("MW:  it=%i, jt=%i", (*it)->name(), (*jt)->name() );
			if  ( (weight=activeGraph.hasEdge((*it)->name(), (*jt)->name()))!=0   &&   (activeGraph.hasEdge((*jt)->name(), (*it)->name()))!=0  )  {
				if ( (*it)->name() > (*jt)->name() ) continue;
			        t << (*it)->name() <<" "<<(*jt)->name()<< " "<<weight;
				t<< " c "<< (*it)->outLinkColor( (*jt)->name() );
          			t <<"\n";
			}

		}
	}
	f.close();
	statusBar()->showMessage( QString(tr( "File %1 saved" ) ).arg( fileNameNoPath.last() ), statusBarDuration );

	return true;
}



/**	Exports the network to a adjacency matrix-formatted file
	Does not preserve node properties nor floating point weights.
*/
bool MainWindow::slotExportSM(){
	qDebug("MW: slotExportSM()");
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("Nothing to export!\nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("Cannot export to Adjacency Matrix.") ,statusBarDuration);
		return false;
	}

	QMessageBox::information(this, "Warning",tr("Note that exporting to an adjacency matrix does not save floating-point weight values; adjacency matrices consist of integers, only. \n If your network had any floating point weights in some edges, these are being truncated to the nearest integer or 1.)"), "OK",0);
	if (fileName.isEmpty()) {
		statusBar()->showMessage(tr("Saving network under new filename..."));
  		QString fn = QFileDialog::getSaveFileName(this, 0, 0);
 	 	if (!fn.isEmpty())  {
			fileName=fn;
  		}
  		else  {
   		 	statusBar()->showMessage(tr("Saving aborted"), statusBarDuration);
			return false;
 		}
	}
	QFile f( fileName );
	if ( !f.open( QIODevice::WriteOnly ) )  {
		statusBar()->showMessage( QString(tr("Could not write to %1")).arg(fileName), statusBarDuration );
		return false;
	}
	QTextStream t( &f );
	qDebug("MW: slotExportSM() for %i activeNodes", activeNodes());

	activeGraph.writeAdjacencyMatrixTo(t);

	f.close();
	statusBar()->showMessage( QString( tr("Adjacency matrix-formatted network saved into file %1") ).arg( fileNameNoPath.last() ), statusBarDuration );
	adjacencyFileLoaded=TRUE;
	pajekFileLoaded=FALSE;
	return true;
}






/**
	Exports the network to a DL-formatted file
	TODO slotExportDL
*/
bool MainWindow::slotExportDL(){
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("Nothing to export!\nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("Cannot export to DL.") ,statusBarDuration);
		return false;
	}

	if (fileName.isEmpty()) {
		statusBar()->showMessage(tr("Saving network under new filename..."));
  		QString fn = QFileDialog::getSaveFileName(this, 0, 0);
 	 	if (!fn.isEmpty())  {
			fileName=fn;
  		}
  		else  {
   		 	statusBar()->showMessage(tr("Saving aborted"), statusBarDuration);
			return false;
 		}
	}

return true;

}


/**
	Exports the network to a GW-formatted file
	TODO slotExportGW
*/ 
bool MainWindow::slotExportGW(){
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("Nothing to export!\nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("Cannot export to GW.") ,statusBarDuration);
		return false;
	}

	if (fileName.isEmpty()) {
		statusBar()->showMessage(tr("Saving network under new filename..."));
  		QString fn = QFileDialog::getSaveFileName(this, 0, 0);
 	 	if (!fn.isEmpty())  {
			fileName=fn;
  		}
  		else  {
   		 	statusBar()->showMessage(tr("Saving aborted"), statusBarDuration);
			return false;
 		}
	}

return true;
}




/**
	Exports the network to a list-formatted file
	TODO slotExportList
*/
bool MainWindow::slotExportList(){
	if (fileName.isEmpty()) {
		statusBar()->showMessage(tr("Saving network under new filename..."));
  		QString fn = QFileDialog::getSaveFileName(this, 0, 0);
 	 	if (!fn.isEmpty())  {
			fileName=fn;
  		}
  		else  {
   		 	statusBar()->showMessage(tr("Saving aborted"), statusBarDuration);
			return false;
 		}
	}

return true;
}





/**
	Adds a little universal randomness :)
*/
void MainWindow::makeThingsLookRandom()   {
    time_t now;				/* define 'now'. time_t is probably a typedef	*/
    now = time((time_t *)NULL);		/* Get the system time and put it
   					 * into 'now' as 'calender time' the number of seconds since  1/1/1970   	*/

     srand( (unsigned int ) now);
  }




/**
	Displays the file of the loaded network.
	Network _must_ be unchanged since last save/load.
	Otherwise it will ask the user to first save the network, then view its file.
*/
void MainWindow::slotViewNetworkFile(){
	qDebug("slotViewNetworkFile()");
	if ( fileLoaded && !networkModified )    { //file network unmodified
		QFile f( fileName );
		if ( !f.open( QIODevice::ReadOnly ) ) {
			qDebug ("Error in open!");
			return;
		}
		TextEditor *ed = new TextEditor(fileName);//OPEN A TEXT EDITOR WINDOW
		ed->setWindowTitle(tr("Viewing network file - ") + fileNameNoPath.last() );
		ed->show();
		statusBar()->showMessage( tr("Loaded network text file " )+ fileNameNoPath.last() , statusBarDuration );
	}
	else if (fileName.isEmpty() && networkModified)     {  //New network + something
		QMessageBox::information (this, "Viewing network file",
		tr("Network not saved yet. I will open a dialog for you to save it now."), "OK",0);
		slotFileSaveAs();
	}
	else if (fileLoaded && networkModified )     {   //file network + modified
		QMessageBox::information (this, "Viewing network file",
		//FIXME maybe better to save automatigally than asking?
		tr("Network has been modified. Please save it now."), "OK",0);
		slotFileSave();
	}

	else	{
		QMessageBox::critical(this, "Error",
		tr("Empty network! \nLoad a network file first or create and save a new one..."), "OK",0);
		statusBar()->showMessage( tr("Nothing here. Not my fault, though!"), statusBarDuration );
	}
}






/**
	Displays the adjacency matrix of the network.
	It uses a different method for writing the matrix to a file.
	While slotExportSM uses << operator of Matrix class (via adjacencyMatrix of Graph class), this is using directly the writeAdjacencyMatrix method of Graph class
*/
void MainWindow::slotViewAdjacencyMatrix(){

	if ( !fileLoaded && !networkModified) {
		QMessageBox::critical (this, "Error",
		tr("Empty network! \nLoad a network file or create something by double-clicking on the canvas!"), "OK",0);

        	statusBar()->showMessage( tr("Nothing to show!"), statusBarDuration );
		return;
	}	
	int aNodes=activeNodes();
	statusBar() ->  showMessage ( QString (tr ("creating adjacency adjacency matrix of %1 nodes")).arg(aNodes), statusBarDuration );
	qDebug ("MW: calling writeAdjacencyMatrix with %i nodes", aNodes);
	char fn[]= "adjacency-matrix.dat";

	activeGraph.writeAdjacencyMatrix(fn, networkName.toLocal8Bit()) ;

	//Open a text editor window for the new file created by graph class
	QString qfn=QString::fromLocal8Bit("adjacency-matrix.dat");
	TextEditor *ed = new TextEditor(fn);
	tempFileNameNoPath=qfn.split( "/");
	ed->setWindowTitle(tr("View Adjacency Matrix - ") + tempFileNameNoPath.last());
	ed->show();

}




/**
	Calls activeGraph.createRandomNetErdos () to create a symmetric network
	Link existance is controlled by a user specified possibility.
*/
void MainWindow::slotCreateRandomNetErdos(){
	bool ok;
	statusBar()->showMessage("You have selected to create a random symmetric network. ", statusBarDuration);
	int newNodes=( QInputDialog::getInteger(this, "Create random network", tr("This will create a new random symmetric network of G(n,p) model, \nwhere n is the nodes and p is the edge probability. \nPlease enter the number n of nodes you want:"),1, 1, maxNodes, 1, &ok ) ) ;
	if (!ok) { 
		statusBar()->showMessage("You did not enter an integer. Aborting.", statusBarDuration);
		return;
	}
	double probability= QInputDialog::getDouble(this,"Create random network", "Enter an edge probability % (0-100):", 0, 0, 100, 1, &ok );
	if (!ok) { 
		statusBar()->showMessage("You did not enter an integer. Aborting.", statusBarDuration);
		return;
	}
	statusBar()->showMessage("Erasing any existing network. ", statusBarDuration);

	initNet();  
	makeThingsLookRandom();  
	statusBar()->showMessage(tr("Creating random network. Please wait... ") ,statusBarDuration);

	qDebug("MW Erdos network:  Create random network of %i nodes and %f edge probability.",newNodes, probability);

	if (showProgressBarAct->isChecked() || newNodes > 300){
		progressDialog= new QProgressDialog("Creating random network. Please wait (or disable me from Options > View > ProgressBar, next time ;)).", "Cancel", 0, newNodes+newNodes, this);
		progressDialog -> setWindowModality(Qt::WindowModal);
		connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
		progressDialog->setMinimumDuration(0);
	}
	
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

	activeGraph.createRandomNetErdos (newNodes, probability);
	QApplication::restoreOverrideCursor();

	if (showProgressBarAct->isChecked() || newNodes > 300)
		progressDialog->deleteLater();	

  	fileContainsNodeColors=FALSE;
	fileContainsLinksColors=FALSE;
	fileContainsNodesCoords=FALSE;

	fileLoaded=false;
	
	graphChanged();
	
	setWindowTitle("Untitled");
	double threshold = log(newNodes)/newNodes;
	float avGraphDistance=activeGraph.averageGraphDistance();
	float clucof=activeGraph.clusteringCoefficient();
	if ( (probability/100 ) > threshold )
		QMessageBox::information(this, "New Random Network",
		tr("Random network created. \n")+ 
		tr("\nNodes: ")+ QString::number(activeNodes())+
		tr("\nEdges: ")+  QString::number( activeLinks()/2.0)+
		tr("\nAverage path length: ") + QString::number(avGraphDistance)+
		tr("\nClustering coefficient: ")+QString::number(clucof)+	
		tr("\n\nOn the average, edges should be ") + QString::number(probability * newNodes*(newNodes-1)/100) +
		tr("\nThis graph is almost surely connected because: \nprobability > ln(n)/n, that is: \n") + QString::number(probability/100)+ 
		tr(" bigger than ")+ QString::number(threshold) , "OK",0);

	else 
		QMessageBox::information(this, "New Random Network",
		tr("Random network created. \n")+ 
		tr("\nNodes: ")+ QString::number(activeNodes())+
		tr("\nEdges: ")+  QString::number( activeLinks()/2.0)+
		tr("\nAverage path length: ") + QString::number(avGraphDistance)+
		tr("\nClustering coefficient: ")+QString::number(clucof)+
 		tr("\n\nOn the average, edges should be ") +QString::number(probability * newNodes*(newNodes-1)/100) +
		tr("\nThis graph is almost surely not connected because: \nprobability < ln(n)/n, that is: \n") +
		QString::number(probability/100)+ " smaller than "+ QString::number(threshold) , "OK",0);

	statusBar()->showMessage("Random network created. ", statusBarDuration);

}










/** TODO 
*/

void MainWindow::slotCreateConnectedRandomNetwork() {
	statusBar()->showMessage("Erasing any existing network. ", statusBarDuration);
	statusBar()->showMessage(tr("Creating uniform random network. Please wait... ") ,statusBarDuration);
}



/**
	Creates a pseudo-random network where every node has the same degree 
*/
void MainWindow::slotCreateSameDegreeRandomNetwork(){
	bool ok;
	statusBar()->showMessage("You have selected to create a pseudo-random network where each node has the same degree. ", statusBarDuration);
	int newNodes=( QInputDialog::getInteger(this, "Create same degree network", tr("This will create a same degree network. \nPlease enter the number of nodes you want:"),1, 1, maxNodes, 1, &ok ) ) ;
	if (!ok) { 
		statusBar()->showMessage("You did not enter an integer. Aborting.", statusBarDuration);
		return;
	}
	int degree = QInputDialog::getInteger(this,"Create same degree network...", "Now, select an even number d. \nThis will be the number of links of each node:", 2, 2, newNodes-1, 2, &ok);
	if ( (degree% 2)==1 ) {
		QMessageBox::critical(this, "Error",tr(" Sorry. I cannot create such a network. Links must be even number"), "OK",0);
		return;
	}
	statusBar()->showMessage("Erasing any existing network. ", statusBarDuration);
	initNet();  
	makeThingsLookRandom();  
	statusBar()->showMessage("Creating a pseudo-random network where each node has the same degree... ", statusBarDuration);

	if (showProgressBarAct->isChecked() || newNodes > 300){
		progressDialog= new QProgressDialog("Creating random network. Please wait (or disable me from Options > View > ProgressBar, next time ;)).", "Cancel", 0, (int) (newNodes+newNodes), this);
		progressDialog -> setWindowModality(Qt::WindowModal);
		connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
		progressDialog->setMinimumDuration(0);
	}

	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

	activeGraph.createSameDegreeRandomNetwork (newNodes, degree);

	QApplication::restoreOverrideCursor();

	if (showProgressBarAct->isChecked() || newNodes > 300)
		progressDialog->deleteLater();	

  	fileContainsNodeColors=FALSE;
	fileContainsLinksColors=FALSE;
	fileContainsNodesCoords=FALSE;

	fileLoaded=false;
	
	graphChanged();
	setWindowTitle("Untitled");
	statusBar()->showMessage("Uniform random network created: "+QString::number(activeNodes())+" Nodes, "+QString::number( activeLinks())+" Links", statusBarDuration);

}


void MainWindow::slotCreateGaussianRandomNetwork(){
	graphChanged();

}



void MainWindow::slotCreateSmallWorldRandomNetwork(){
	bool ok=false;
	statusBar()->showMessage("You have selected to create a small world network.", statusBarDuration);
	int newNodes=( QInputDialog::getInteger(this, "Create small world", 
			tr("This will create a small world network, \n")+
			tr("that is an undirected graph with N nodes and N*d/2 edges,\n")+
			tr("where d is the mean edge degree.\n")+
			tr("Please enter the number N of nodes you want:"),
			1, 1, maxNodes, 1, &ok ) ) ;
	if (!ok) { 
		statusBar()->showMessage("You did not enter an integer. Aborting.", statusBarDuration);
		return;
	}
	int degree = QInputDialog::getInteger(this,"Create small world...",
			 tr("Now, enter an even number d. \n")+
			 tr("This is the mean edge degree each new node will have:"), 
			 2, 2, newNodes-1, 2, &ok);
	if ( (degree% 2)==1 ) {
		QMessageBox::critical(this, "Error",tr(" Sorry. I cannot create such a network. Links must be even number"), "OK",0);
		return;
	}
	double beta = QInputDialog::getDouble(this,"Create small world...",
			 tr("Now, enter a parameter beta. \n")+
			 tr("This is the edge rewiring probability:"), 
			 0.52, 0.00, 1.00, 2, &ok);

	statusBar()->showMessage(tr("Erasing any existing network. "), statusBarDuration);
	initNet();  
	makeThingsLookRandom();  
	statusBar()->showMessage(tr("Creating small world. Please wait..."), statusBarDuration);
	double x0=scene->width()/2.0;
	double y0=scene->height()/2.0;
	double radius=(graphicsWidget->height()/2.0)-50;          //pixels

	if (showProgressBarAct->isChecked()){
		progressDialog= new QProgressDialog(
			tr("Creating random network. Please wait \n (or disable me from Options > View > ProgressBar, next time )."), 
			"Cancel", 0, (int) (newNodes+newNodes), this);
		progressDialog -> setWindowModality(Qt::WindowModal);
		connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
		progressDialog->setMinimumDuration(0);
	}
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	activeGraph.createRandomNetSmallWorld(newNodes, degree, beta, x0, y0, radius);
	QApplication::restoreOverrideCursor();

	if (showProgressBarAct->isChecked())
		progressDialog->deleteLater();	

  	fileContainsNodeColors=FALSE;
	fileContainsLinksColors=FALSE;
	fileContainsNodesCoords=FALSE;

	fileLoaded=false;

	graphChanged();
	setWindowTitle("Untitled");
	statusBar()->showMessage(tr("Small world random network created: ")+QString::number(activeNodes())+" nodes, "+QString::number( activeLinks())+" links", statusBarDuration);
	float avGraphDistance=activeGraph.averageGraphDistance();
	float clucof=activeGraph.clusteringCoefficient();
	QMessageBox::information(this, "New Small World",
		tr("Small world network created.\n")+ 
		tr("\nNodes: ")+ QString::number(activeNodes())+
		tr("\nEdges: ")+  QString::number( activeLinks()/2.0)+ 
		tr("\nAverage path length: ") + QString::number(avGraphDistance)+
		tr("\nClustering coefficient: ")+QString::number(clucof)	
		 , "OK",0);
		
}





/**
	Creates a lattice network, i.e. a connected network where every node
	has the same degree and is linked with its neighborhood.
*/
void MainWindow::slotCreateRandomNetRingLattice(){
	bool ok;
	statusBar()->showMessage("You have selected to create a ring lattice network. ", statusBarDuration);
	int newNodes=( QInputDialog::getInteger(this, "Create ring lattice", tr("This will create a ring lattice network, where each node has degree d:\n d/2 edges to the right and d/2 to the left.\n Please enter the number of nodes you want:"),1, 1, maxNodes, 1, &ok ) ) ;
	if (!ok) { 
		statusBar()->showMessage("You did not enter an integer. Aborting.", statusBarDuration);
		return;
	}
	int degree = QInputDialog::getInteger(this,"Create ring lattice...", "Now, enter an even number d. \nThis is the total number of links each new node will have:", 2, 2, newNodes-1, 2, &ok);
	if ( (degree% 2)==1 ) {
		QMessageBox::critical(this, "Error",tr(" Sorry. I cannot create such a network. Links must be even number"), "OK",0);
		return;
	}

	statusBar()->showMessage("Erasing any existing network. ", statusBarDuration);
	initNet();  
	makeThingsLookRandom();  
	statusBar()->showMessage("Creating ring lattice network. Please wait...", statusBarDuration);
	double x0=scene->width()/2.0;
	double y0=scene->height()/2.0;
	double radius=(graphicsWidget->height()/2.0)-50;          //pixels

	if (showProgressBarAct->isChecked()){
		progressDialog= new QProgressDialog("Creating random network. Please wait (or disable me from Options > View > ProgressBar, next time ;)).", "Cancel", 0, (int) (newNodes+newNodes), this);
		progressDialog -> setWindowModality(Qt::WindowModal);
		connect( &activeGraph, SIGNAL( updateProgressDialog(int) ), progressDialog, SLOT(setValue(int) ) ) ;
		progressDialog->setMinimumDuration(0);
	}
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

	activeGraph.createRandomNetRingLattice(newNodes, degree, x0, y0, radius);

	QApplication::restoreOverrideCursor();

	if (showProgressBarAct->isChecked())
		progressDialog->deleteLater();	

 	fileContainsNodeColors=FALSE;
	fileContainsLinksColors=FALSE;
	fileContainsNodesCoords=FALSE;

	fileLoaded=false;
	
	graphChanged();
	setWindowTitle("Untitled");
	float avGraphDistance=activeGraph.averageGraphDistance();
	float clucof=activeGraph.clusteringCoefficient();
	QMessageBox::information(this, "Ring Lattice",
		tr("Ring lattice network created.\n")+ 
		tr("\nNodes: ")+ QString::number(activeNodes())+
		tr("\nEdges: ")+  QString::number( activeLinks()/2.0)+ 
		tr("\nAverage path length: ") + QString::number(avGraphDistance)+
		tr("\nClustering coefficient: ")+QString::number(clucof)	
		 , "OK",0);

	statusBar()->showMessage("Ring lattice random network created: "+QString::number(activeNodes())+" nodes, "+QString::number( activeLinks())+" links", statusBarDuration);	
}








/**
	 Finds and marks (by double-sizing and highlighting) a node by its number.
*/
void MainWindow::slotFindNode(){
	qDebug ("MW: slotFindNode()");
	if (markedNodeExists) {
		markedNode->setSize(preSize);
		markedNode->setColor ( QColor( markedNode->color() ).dark(150) );
		markedNodeExists=FALSE;
		return;
	}
	if (!fileLoaded && !networkModified  )     {
		QMessageBox::critical( this, "Find Node",
				      tr("No nodes present! \nLoad a network file first or create some nodes..."), tr("OK"),0 );
		statusBar()->showMessage( QString(tr("Nothing to find!")) , statusBarDuration);
		return;
	}
	bool ok=FALSE;
	QString text = QInputDialog::getText(this, "Find a node", tr("Enter node label or number:"), QLineEdit::Normal,QString::null, &ok );
	if (!ok) {
		statusBar()->showMessage("Find node operation cancelled.", statusBarDuration);
		return;
	}

	QList<QGraphicsItem *> list=scene->items();
	for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++) {
		if ( (*it)->type()==TypeNode) {
			Node *jim=(Node*) (*it);
			if ( jim->nodeNumber()==text.toInt(&ok, 10))	{
				qDebug("MW: found.");
				jim->setColor ( QColor( jim->color() ).light(150) );
				preSize=jim->size();
				jim->setSize(2*preSize-1);
				markedNodeExists=TRUE;
				markedNode=jim;
				statusBar()->showMessage(tr("Node found!"));
				return;
				
			}
			if (jim->label().contains (text, Qt::CaseInsensitive) ) {
				preSize=jim->size();
				jim->setSize(2*preSize-1);
				qDebug("MW: found.");
				jim->setColor ( QColor( jim->color() ).light(150) );
				markedNodeExists=TRUE;
				markedNode=jim;
				statusBar()->showMessage(tr("Node found!"));
				return;
			}
		}
	}
	QMessageBox::information(this, "Find Node", tr("Sorry. There is no such node in this network. \n Try again."), "OK",0);

}



/**
	 Popups a context menu with some options when the user right-clicks on a node
*/
void MainWindow::openNodeContextMenu() {
	clickedJimNumber=clickedJim->nodeNumber();
	qDebug("MW: openNodeContextMenu() for node %i at %i, %i",clickedJimNumber, QCursor::pos().x(), QCursor::pos().y());

	QMenu *nodeContextMenu = new QMenu(QString::number(clickedJimNumber), this);
	Q_CHECK_PTR( nodeContextMenu );  //displays "out of memory" if needed

	nodeContextMenu -> addAction(addLinkAct);
	nodeContextMenu -> addAction(removeNodeAct );
	QMenu *options=new QMenu(tr("Options"), this);
	nodeContextMenu -> addMenu( options );
	options -> addAction( changeNodeLabelAct );
	options -> addAction( changeNodeSizeAct );
	options -> addAction( changeNodeValueAct );
	options -> addAction( changeNodeColorAct );
	options -> addAction( changeNodeBoxAct );
	options -> addAction( changeNodeCircleAct );
	options -> addAction( changeNodeDiamondAct );
	options -> addAction( changeNodeEllipseAct);
	options -> addAction( changeNodeTriangleAct );
	//QCursor::pos() is good only for menus not related with node coordinates
	nodeContextMenu -> exec(QCursor::pos() );
	delete  nodeContextMenu;
	clickedJimNumber=-1;    //undo node selection
}



/**
	 Popups a context menu with some options when the user right-clicks on a link
*/
void MainWindow::openLinkContextMenu() {
	int source=clickedLink->sourceNodeNumber();
	int target=clickedLink->targetNodeNumber();
	qDebug("MW: openLinkContextMenu() for edge %i-%i at %i, %i",source, target, QCursor::pos().x(), QCursor::pos().y());
	QString edgeName=QString::number(source)+QString("-")+QString::number(target);
	//make the menu
	QMenu *linkContextMenu = new QMenu(edgeName, this);
	linkContextMenu -> addAction( removeLinkAct );
	linkContextMenu -> addAction( changeLinkWeightAct );
	linkContextMenu -> addAction( changeLinkColorAct );
	linkContextMenu -> exec(QCursor::pos() );
	delete  linkContextMenu;
}

/**
	 Popups a context menu with some options when the user right-clicks on the scene
*/
void MainWindow::openContextMenu( const QPointF &mPos) {
	cursorPosGW=mPos;
	QMenu *contextMenu = new QMenu("Link Menu",this);
	Q_CHECK_PTR( contextMenu );  //displays "out of memory" if needed
	contextMenu -> addAction( addNodeAct );
	QMenu *options=new QMenu("Options", this);
	contextMenu -> addMenu(options );
	options -> addAction(changeBackColorAct  );
	options -> addAction(changeAllNodesSizeAct );
	options -> addAction(changeAllNodesShapeAct  );
	options -> addAction(changeAllNodesColorAct );
	options -> addAction(changeAllLinksColorAct  );
	options -> addAction (displayNodeNumbersAct);
	options -> addAction (displayNodeLabelsAct);
	//QCursor::pos() is good only for menus not related with node coordinates
	contextMenu -> exec(QCursor::pos() );
	delete  contextMenu;
	cursorPosGW=QPoint(-1,-1);
}







/**
*	A slot activated when something has been changed in the graph.
	Makes the fileSave icon active and refreshes any LCD values. 
	Also called from graphicsWidget.
*/
void MainWindow::graphChanged(){
	qDebug("MW: graphChanged");
	networkModified=TRUE;
	fileSave->setIcon(QIcon(":/images/save.png"));
	fileSave->setEnabled(true);
	
	nodesLCD->display(activeGraph.vertices());
	edgesLCD->display(activeGraph.totalEdges());
	densityLCD->display( activeGraph.density() );

	inLinkedNodesLCD -> display(activeGraph.verticesWithInEdges());
	outLinkedNodesLCD-> display(activeGraph.verticesWithOutEdges());
	reciprocalLinkedNodesLCD -> display(activeGraph.verticesWithReciprocalEdges());
}



/**
*	When the user clicks on a node, displays some information about it on the status bar.
*/
void MainWindow::nodeInfoStatusBar ( Node *jim) {
	qDebug ("MW: NodeInfoStatusBar()");	
	linkClicked=FALSE;
	nodeClicked=TRUE;
	clickedJim=jim;
	clickedJimNumber=clickedJim->nodeNumber();
	int inLinks=activeGraph.edgesTo(clickedJimNumber);
	int outLinks=activeGraph.edgesFrom(clickedJimNumber);
	selectedNodeLCD->display (clickedJimNumber);
	inLinksLCD->display (inLinks);
	outLinksLCD->display (outLinks);
	clucofLCD->display(activeGraph.clusteringCoefficient(clickedJimNumber));
	
	statusBar()->showMessage( QString(tr("(%1, %2);  Node %3, with label %4, "
		"has %5 in-Links and %6 out-Links.")).arg( ceil( clickedJim->x() ) )
		.arg( ceil( clickedJim->y() )).arg( clickedJimNumber ).arg( clickedJim->label() )
		.arg(inLinks).arg(outLinks), statusBarDuration);
	clickedJimNumber=-1;
}



/**
*	When the user clicks on a link, displays some information about it on the status bar.
*/
void MainWindow::linkInfoStatusBar (Edge* link) {
	clickedLink=link;
	linkClicked=TRUE;
	nodeClicked=FALSE;
	if (bezier)
		statusBar()->showMessage( QString(tr("Link from Node %1 to Node %2, has weight %3 and color %4.")).arg( link->sourceNodeNumber() ).arg(link->targetNodeNumber()).arg(link->weight()).arg(link->color() ), statusBarDuration);
	else
		statusBar()->showMessage( QString(tr("Link between node %1 and node %2, weight %3 and color %4.")).arg( link->sourceNodeNumber() ).arg(link->targetNodeNumber()).arg(link->weight()).arg(link->color() ), statusBarDuration);
}







/**
* 	Deletes a node and the attached objects (links, etc).
*	It deletes clickedJim (signal from GraphicsView or set by another function) 
*	or else asks for a nodeNumber to remove. The nodeNumber is doomedJim.
	Called from nodeContextMenu
*/
void MainWindow::slotRemoveNode() {
	qDebug("MW: slotRemoveNode()");
	if (!activeGraph.vertices())  {
		QMessageBox::critical(this, "Error",tr("Nothing to do! \nLoad a network file or add some nodes first."), "OK",0);
		statusBar()->showMessage(tr("Nothing to remove.") ,statusBarDuration);
		return;
	}
	int doomedJim=-1, min=-1, max=-1;
	bool ok=false; 

	min = activeGraph.firstVertexNumber();
	max = activeGraph.lastVertexNumber();
	qDebug("MW: min is %i and max is %i", min, max);

	if (min==-1 || max==-1 ) { qDebug("ERROR in finding min max nodeNumbers. Abort"); return;}	
	else if (clickedJimNumber >= 0 && clickedJimNumber<= max ) { 
		doomedJim=clickedJimNumber ;
	}
	else if (clickedJimNumber == -1 ) {
		doomedJim =  QInputDialog::getInteger(this,"Remove node",tr("Choose a node to remove between ("
			+QString::number(min).toAscii()+"..."+QString::number(max).toAscii()+"):"),min, 1, max, 1, &ok);
		if (!ok) {
			statusBar()->showMessage("Remove node operation cancelled.", statusBarDuration);
			return;
		}
	}
	qDebug("MW: clickedJimNumber is %i. Deleting node %i from GraphicsWidget", clickedJimNumber, doomedJim);
	graphicsWidget->removeNode(doomedJim);
	qDebug ("MW: removing vertice with number %i from activeGraph", doomedJim);
	activeGraph.removeVertex(doomedJim);
	clickedJimNumber=-1;
	graphChanged();
	qDebug("MW: removeNode() completed. Node %i removed completely.",doomedJim);
	statusBar()->showMessage(tr("Node removed completely. Ready. "),statusBarDuration);
}





/**
*	Adds a new link between two nodes specified by the user.
	Called when user clicks on the MW button "Add Link".
*/
void MainWindow::slotAddLink(){
	qDebug ("MW: slotAddLink()");
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("Nothing to link to! \nCreate some nodes first."), "OK",0);
		statusBar()->showMessage(tr("There are no nodes yet...") ,statusBarDuration);
		return;
	}

	int sourceNode=-1, targetNode=-1, sourceIndex=-1, targetIndex=-1;
	float weight=1; 	//weight of this new edge should be one...
	bool ok=FALSE;
	int min=activeGraph.firstVertexNumber();
	int max=activeGraph.lastVertexNumber();

	if (min==max) return;		//if there is only one node -> no link 

	if (clickedJimNumber == -1) {
		sourceNode=QInputDialog::getInteger(this, "Create new link, Step 1",tr("This will draw a new link between two nodes. \nEnter source node ("+QString::number(min).toAscii()+"..."+QString::number(max).toAscii()+"):"), min, 1, max , 1, &ok ) ;
		if (!ok) {
			statusBar()->showMessage("Add link operation cancelled.", statusBarDuration);
			return;
		}
	}
	else sourceNode=clickedJimNumber;
	
	if ( (sourceIndex =activeGraph.hasVertex(sourceNode)) ==-1 ) {
		statusBar()->showMessage(tr("Aborting. ") ,statusBarDuration);
		QMessageBox::critical(this,"Error","No such node.", "OK",0);
		qDebug ("MW: slotAddLink: Cant find sourceNode %i.", sourceNode);
		return;
	}
	
	targetNode=QInputDialog::getInteger(this, "Create new link, Step 2", tr("Source node accepted. \nNow enter target node ("+QString::number(min).toAscii()+"..."+QString::number(max).toAscii()+"):"),min, 1, max , 1, &ok)     ;
	if (!ok) {
		statusBar()->showMessage("Add link target operation cancelled.", statusBarDuration);
		return;
	}
	if ( (targetIndex=activeGraph.hasVertex(targetNode)) ==-1 ) {
		statusBar()->showMessage(tr("Aborting. ") ,statusBarDuration);	
		QMessageBox::critical(this,"Error","No such node.", "OK",0);
		qDebug ("MW: slotAddLink: Cant find targetNode %i",targetNode);
		return;
	}
	
	weight=QInputDialog::getDouble(this, "Create new link, Step 3", tr("Source and target nodes accepted. \n Please, enter the weight of new link: "),1.0, -20.0, 20.0, 1, &ok);
	if (!ok) {
		statusBar()->showMessage("Add link operation cancelled.", statusBarDuration);
		return;
	}
	//Check if this link already exists...
	if (activeGraph.hasEdge(sourceNode, targetNode)!=0 ) {
		qDebug("Link exists. Aborting");
		statusBar()->showMessage(tr("Aborting. ") ,statusBarDuration);
		QMessageBox::critical(this,"Error","Link already exists.", "OK",0);
		return;
	}

	addLink(sourceNode, targetNode, weight);
	graphChanged();
	statusBar()->showMessage(tr("Ready. ") ,statusBarDuration);
}



/** 	
	helper to slotAddLink() above
	Also called from GW::userMiddleClicked() signal when user creates links with middle-clicks
	Calls Graph::createEdge method to add the new edge to the active Graph 
*/
void MainWindow::addLink (int v1, int v2, float weight) {
	qDebug("MW: addLink() - setting user preferences and calling Graph::createEdge(...)");
	bool drawArrows=displayLinksArrowsAct->isChecked();
	bool reciprocal=false;
	bool bezier = false;
	activeGraph.createEdge(v1, v2, weight, reciprocal, drawArrows, bezier);
}



/**
*	Erases the clicked link. Otherwise asks the user to specify one link.
*	First deletes arc reference from object nodeVector
*	then deletes arc item from scene
**/
void MainWindow::slotRemoveLink(){ 
	if ( (!fileLoaded && !networkModified) || activeGraph.totalEdges() ==0 )  {
		QMessageBox::critical(this, "Error",tr("No links present! \nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("No links to remove - sorry.") ,statusBarDuration);
		return;
	}

	int min=0, max=0, sourceNode=-1, targetNode=-1;
	bool ok=FALSE;
	min=activeGraph.firstVertexNumber();
	max=activeGraph.lastVertexNumber();

  	if (!linkClicked) {
		sourceNode=QInputDialog::getInteger(this,tr("Remove link"),tr("Source node:  (")+QString::number(min)+"..."+QString::number(max)+"):", min, 1, max , 1, &ok )   ;
		if (!ok) {
			statusBar()->showMessage("Remove link operation cancelled.", statusBarDuration);
			return;
		}

		targetNode=QInputDialog::getInteger(this, tr("Remove link"), tr("Target node:  (")+QString::number(min)+"..."+QString::number(max)+"):",min, 1, max , 1, &ok )   ;
		if (!ok) {
			statusBar()->showMessage("Remove link operation cancelled.", statusBarDuration);
			return;
		}
		if ( activeGraph.hasEdge(sourceNode, targetNode)!=0 ) {
			if (activeGraph.symmetricEdge(sourceNode, targetNode) ) 
				graphicsWidget->unmakeEdgeReciprocal(targetNode, sourceNode);
			graphicsWidget->eraseEdge(sourceNode, targetNode);
			activeGraph.removeEdge(sourceNode, targetNode);
		}
		else {
			QMessageBox::critical(this, "Remove link",tr("There is no such link."), "OK",0);
			statusBar()->showMessage(tr("There are no nodes yet...") ,statusBarDuration);
			return;
		}

	}
	else {
		sourceNode = clickedLink->sourceNodeNumber();
		targetNode = clickedLink->targetNodeNumber();
		if (activeGraph.symmetricEdge(sourceNode, targetNode) ) {
			QString s=QString::number(sourceNode);
			QString t=QString::number(targetNode);
			switch (QMessageBox::information( this, tr("Remove link"),
				      tr("This link is reciprocal. \n") +
				      tr("Select what Direction to delete or Both..."),
				      s+" -> "+ t, t+" -> "+s, tr("Both"), 0, 1 ))

				{
					case 0:
						graphicsWidget->removeItem(clickedLink);
						activeGraph.removeEdge(sourceNode, targetNode);
						//make new link
// 						graphicsWidget->unmakeEdgeReciprocal(clickedLink->targetNodeNumber(), clickedLink->sourceNodeNumber());
						//FIXME weight should be the same 
						graphicsWidget->drawEdge(targetNode, sourceNode, 1, false, displayLinksArrowsAct->isChecked(), initLinkColor, false, false);

						break;
					case 1:
						clickedLink->unmakeReciprocal();  
						//graphicsWidget->removeItem(clickedLink);
						activeGraph.removeEdge(targetNode, sourceNode);
//						graphicsWidget->drawEdge(i, j, false, drawArrowsAct->isChecked(), initLinkColor, false, false);
						break;
					case 2:
						graphicsWidget->removeItem(clickedLink);
						activeGraph.removeEdge(sourceNode, targetNode);
						activeGraph.removeEdge(targetNode, sourceNode);
				}


		}
		else {
			graphicsWidget->removeItem(clickedLink);
			activeGraph.removeEdge(sourceNode, targetNode);

			}

		
	}
	graphChanged();
	qDebug("MW: View items now: %i ", graphicsWidget->items().size());
	qDebug("MW: Scene items now: %i ", scene->items().size());
}




/**
*	Changes the label of the clicked node
*/
void MainWindow::slotChangeNodeLabel(){
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("There are no nodes! \nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("No nodes created.") ,statusBarDuration);
		return;
	}
	if (clickedJimNumber==-1) {
		statusBar()->showMessage(tr("Please click on a node first... ") ,statusBarDuration);
		return;
	}
	bool ok;
	QString text = QInputDialog::getText(this, "Change node label", tr("Enter new node label:"), QLineEdit::Normal,
	QString::null, &ok );
	if ( ok && !text.isEmpty() ) {
		qDebug()<<"MW: change label to "<< text.toAscii();
		clickedJim->setLabel(text);
		activeGraph.setVertexLabel( clickedJimNumber, text);
		if (!showLabels()) 
			displayNodeLabelsAct->setChecked(TRUE);
			statusBar()->showMessage(tr("Changed label to %1. Ready. ").arg(text) ,statusBarDuration);
			graphChanged();
	} 
	else {
		statusBar()->showMessage(tr("No label text. Abort. ") ,statusBarDuration);
	}
}




/**
*	Changes the colour of the clicked node
*/
void MainWindow::slotChangeNodeColor(){
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("There are no nodes! \nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("No nodes...") ,statusBarDuration);
		return;
	}

	bool ok;
	if (clickedJimNumber==-1) {
		int min=activeGraph.firstVertexNumber();
		int max=activeGraph.lastVertexNumber();
		int node=QInputDialog::getInteger(this, "Change node color",tr("Select node:  	("+QString::number(min).toAscii()+"..."+QString::number(max).toAscii()+"):"), min, 1, max , 1, &ok)   ;
		statusBar()->showMessage(tr("Error. ") ,statusBarDuration);
		if (!ok) {
			statusBar()->showMessage("Change clicked node color operation cancelled.", statusBarDuration);
			return;
		}

		QString newColor = QInputDialog::getItem(this,"Change node color", "Select a  new color:", colorList, 1, TRUE, &ok);
		if (!ok) {
			statusBar()->showMessage("Change clicked node color operation cancelled.", statusBarDuration);
			return;
		}

		if (graphicsWidget->setNodeColor(node, newColor)) {
			activeGraph.setVertexColor( node, newColor);
			graphChanged();
		}
		else
			statusBar()->showMessage(tr("There is no such link. "), statusBarDuration);
		
	}
	else{
		QString nodeColor = QInputDialog::getItem(this,"Change node color", "Select a  color:", colorList, 1, TRUE, &ok);
    		if ( ok ) {
			clickedJim->setColor(nodeColor);
			activeGraph.setVertexColor (clickedJimNumber, nodeColor);
			graphChanged();
			statusBar()->showMessage(tr("Ready. "), statusBarDuration);
    		} 
		else {
		        // user pressed Cancel
			statusBar()->showMessage(tr("Change node color aborted. "),statusBarDuration);
    		}
	}
}



/**
*	Changes the size of the clicked node.  
*/
void MainWindow::slotChangeNodeSize(){
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("There are no nodes! \nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("Cannot change nothing.") ,statusBarDuration);
		return;
	}

//TODO nodeSize = f(nodeValue) ...
	if (clickedJimNumber==-1) { 
		statusBar()->showMessage(tr("Error. ") ,statusBarDuration);
		return;
	}
	bool ok=FALSE;
	int newSize = QInputDialog::getInteger(this, "Change node size", tr("Change node size to: (1-16)"),initNodeSize, 1, 16, 1, &ok ) ;
	if (!ok) {
		statusBar()->showMessage("Change size operation cancelled.", statusBarDuration);
		return;
	}
	clickedJim->setSize(newSize);
	graphChanged();
	statusBar()->showMessage (QString(tr("Ready")), statusBarDuration) ;
	return;
}



/**
*	TODO Change the value of the clicked node.  
*/
void MainWindow::slotChangeNodeValue(){
	if (clickedJimNumber==-1) { 
		statusBar()->showMessage(tr("Error. ") ,statusBarDuration);
		return;
	}
//	bool ok=FALSE;
	//int newSize =   QInputDialog::getInteger(this, "Change node value", tr("Change node size to: (1-16)"),1, 1, 16, 1, &ok ) ;
//	clickedJim->setSize(newSize);
	graphChanged();
	statusBar()->showMessage (QString(tr("Ready")), statusBarDuration) ;
	return;
}



/**
*	Changes the shape of the clicked node to box
*/
void MainWindow::slotChangeNodeBox(){
	activeGraph.setVertexShape( clickedJim->nodeNumber(), "box");
	clickedJim->setShape("box");
	graphChanged();
}



/**
*	Changes the shape of the clicked node to triangle
*/
void MainWindow::slotChangeNodeTriangle(){
	activeGraph.setVertexShape( clickedJim->nodeNumber(), "triangle");
	clickedJim->setShape("triangle");
	graphChanged();
}



/**
*	Changes the shape of the clicked node to circle
*/
void MainWindow::slotChangeNodeCircle(){
	activeGraph.setVertexShape( clickedJim->nodeNumber(), "circle");
	clickedJim->setShape("circle");
	graphChanged();
}



/**
*	Changes the shape of the clicked node to diamond
*/
void MainWindow::slotChangeNodeDiamond(){
	activeGraph.setVertexShape( clickedJim->nodeNumber(), "diamond");
	clickedJim->setShape("diamond");
	graphChanged();
}



/**
*	Changes the shape of the clicked node to ellipse
*/
void MainWindow::slotChangeNodeEllipse(){
	activeGraph.setVertexShape( clickedJim->nodeNumber(), "ellipse");
	clickedJim->setShape("ellipse");
	graphChanged();
}

 
//TODO slotChangeLinkLabel
void MainWindow::slotChangeLinkLabel(){
	graphChanged();
}



/**
*	Changes the colour of the clicked link. 
*	If no link is clicked, then it asks the user to specify one.
*/
void MainWindow::slotChangeLinkColor(){
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("No links here! \nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("No links present...") ,statusBarDuration);
		return;
	}

	int sourceNode=-1, targetNode=-1;
	bool ok=FALSE;
	QString newColor;
	int min=activeGraph.firstVertexNumber();
	int max=activeGraph.lastVertexNumber();
	
  	if (!linkClicked) {	//no edge clicked. Ask user to define an edge.
		sourceNode=QInputDialog::getInteger(this, "Change link color",tr("Select link source node:  ("+QString::number(min).toAscii()+"..."+QString::number(max).toAscii()+"):"), min, 1, max , 1, &ok)   ;
		if (!ok) {
			statusBar()->showMessage("Change link color operation cancelled.", statusBarDuration);
			return;
		}
		targetNode=QInputDialog::getInteger(this, "Change link color...", tr("Select link target node:  ("+QString::number(min).toAscii()+"..."+QString::number(max).toAscii()+"):"),min, 1, max , 1, &ok  )   ;
		if (!ok) {
			statusBar()->showMessage("Change link color operation cancelled.", statusBarDuration);
			return;
		}

		qDebug("source %i target %i",sourceNode, targetNode);

		newColor = QInputDialog::getItem(this , "Change link color....", "Select a  color:", colorList, 1, FALSE, &ok );
		if ( ok ) {
			if (graphicsWidget->setEdgeColor(sourceNode, targetNode, newColor))
				activeGraph.setEdgeColor( sourceNode, targetNode, newColor);
			else
				statusBar()->showMessage(tr("There is no such link. "), statusBarDuration);
		}
		else	
			statusBar()->showMessage(tr("Change link color cancelled. "), statusBarDuration);

	}
	else {	//edge has been clicked. Just ask the color and call the appropriate methods.
		newColor = QInputDialog::getItem(this, "Change link color....", "Select a new color for the clicked link:", colorList, 1, FALSE, &ok );
		if ( ok ) {
			clickedLink->setColor(newColor);
			activeGraph.setEdgeColor( clickedLink->sourceNodeNumber(), clickedLink->targetNodeNumber(), newColor);
			statusBar()->showMessage(tr("Ready. ") ,statusBarDuration);
			
    		} 
		else {       // user pressed Cancel
			statusBar()->showMessage(tr("User abort. ") ,statusBarDuration);
		}
	}
}




/**
*	Changes the weight of the clicked link. 
*	If no link is clicked, asks the user to specify a link.
*/
void MainWindow::slotChangeLinkWeight(){
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("There are no links here! \nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("No links present...") ,statusBarDuration);
		return;
	}

	qDebug("MW: slotChangeLinkWeight()");
	int  sourceNode=-1, targetNode=-1;
	double newWeight=1.0;
	int min=activeGraph.firstVertexNumber();
	int max=activeGraph.lastVertexNumber();

	bool ok=FALSE;
	if (!linkClicked) {	
		sourceNode=QInputDialog::getInteger(this, "Change link weight",tr("Select link source node:  ("+QString::number(min).toAscii()+"..."+QString::number(max).toAscii()+"):"), min, 1, max , 1, &ok)   ;
		if (!ok) {
			statusBar()->showMessage("Change link weight operation cancelled.", statusBarDuration);
			return;
		}

		targetNode=QInputDialog::getInteger(this, "Change link weight...", tr("Select link target node:  ("+QString::number(min).toAscii()+"..."+QString::number(max).toAscii()+"):"),min, 1, max , 1, &ok  )   ;
		if (!ok) {
			statusBar()->showMessage("Change link weight operation cancelled.", statusBarDuration);
			return;
		}

		qDebug("source %i target %i",sourceNode, targetNode);
		
		QList<QGraphicsItem *> list=scene->items();	
		for (QList<QGraphicsItem *>::iterator it=list.begin(); it!= list.end() ; it++)
			if ( (*it)->type()==TypeEdge) {
				Edge *link=(Edge*) (*it);
				qDebug ("MW: searching link...");
				if ( link->sourceNodeNumber()==sourceNode && link->targetNodeNumber()==targetNode ) {
					qDebug("MW: link found");
					newWeight=QInputDialog::getDouble(this, "Change link weight...",tr("New link Weight: "), 1, -100, 100 ,1, &ok ) ;
					if (ok) {
						link->setWeight(newWeight);
						link->update();
						activeGraph.setEdgeWeight(sourceNode, targetNode, newWeight);
						graphChanged();
						statusBar()->showMessage( QString(tr("Ready.")) ,statusBarDuration);
						return;
					}
					else {
						statusBar()->showMessage( QString(tr("input error. Abort.")) , statusBarDuration);
						return;
					}
				}
			}
	}
	else {  //linkClicked
		sourceNode=clickedLink->sourceNodeNumber();
		targetNode=clickedLink->targetNodeNumber();
		if (activeGraph.symmetricEdge(sourceNode, targetNode) ) {
			QString s=QString::number(sourceNode);
			QString t=QString::number(targetNode);
			switch (QMessageBox::information( this, tr("Change link weight"),
				      tr("This link is reciprocal. \n") +
				      tr("Select what Direction to change or Both..."),
				      s+" -> "+ t, t+" -> "+s, tr("Both"), 0, 1 ))
				{
					case 0:
						qDebug("MW: slotChangeLinkWeight()  %i -> %i", sourceNode, targetNode);
						newWeight=QInputDialog::getInteger(this, "Change link weight...",tr("New link weight: "), 1, -100, 100 ,1, &ok) ;
						if (ok) {
							clickedLink->setWeight(newWeight);
							clickedLink->update();	
							qDebug()<<"MW: newWeight will be "<< newWeight; 
							activeGraph.setEdgeWeight(sourceNode, targetNode, newWeight);
							statusBar()->showMessage( QString(tr("Ready.")) ,statusBarDuration);
							graphChanged();
							return;
						}
						else {
							statusBar()->showMessage( QString(tr("Change link weight cancelled.")) , statusBarDuration);
							return;
						}
						break;
					case 1:
						qDebug("MW: slotChangeLinkWeight()  %i -> %i",targetNode , sourceNode);
						newWeight=QInputDialog::getInteger(this, "Change link weight...",tr("New link weight: "), 1, -100, 100 ,1, &ok) ;
						if (ok) {
							if (graphicsWidget->setEdgeWeight( targetNode, sourceNode, newWeight) ) {
								qDebug()<<"MW: newWeight will be "<< newWeight; 
								activeGraph.setEdgeWeight( targetNode, sourceNode, newWeight);
								statusBar()->showMessage( QString(tr("Ready.")) ,statusBarDuration);
								graphChanged();
							}
							else statusBar()->showMessage( QString(tr("Weight not changed. Sorry.")) ,statusBarDuration);
								
							return;
						}
						else {
							statusBar()->showMessage( QString(tr("Change link weight cancelled.")) , statusBarDuration);
							return;
						}
						break;
					case 2:
						qDebug("MW: slotChangeLinkWeight()  both directions %i <-> %i",targetNode , sourceNode);
						newWeight=QInputDialog::getInteger(this, "Change link weight...",tr("New link weight: "), 1, -100, 100 ,1, &ok) ;
						if (ok) {
							if (
								graphicsWidget->setEdgeWeight( targetNode, sourceNode, newWeight) &&
								graphicsWidget->setEdgeWeight( sourceNode, targetNode, newWeight)
								)  	{
								qDebug()<<"MW: Changing first direction. NewWeight will be "<< newWeight;
								activeGraph.setEdgeWeight(sourceNode, targetNode, newWeight);
								qDebug()<<"MW: Changing opposite direction. NewWeight will be "<< newWeight; 
								activeGraph.setEdgeWeight( targetNode, sourceNode, newWeight);
								statusBar()->showMessage( QString(tr("Ready.")) ,statusBarDuration);
								graphChanged();
							}
							else 
								statusBar()->showMessage( QString(tr("Weight not changed. Sorry.")) ,statusBarDuration);
			 				return;
						}
						else {
							statusBar()->showMessage( QString(tr("Change link weight cancelled.")) , statusBarDuration);
							return;
						}
						break;
				}
		}
		else {
			qDebug("MW: slotChangeLinkWeight()  %i -> %i", sourceNode, targetNode);
			newWeight=QInputDialog::getInteger(this, "Change link weight...",tr("New link weight: "), 1, -100, 100 ,1, &ok) ;
			if (ok) {
				clickedLink->setWeight(newWeight);
				clickedLink->update();	
				qDebug()<<"MW: newWeight will be "<< newWeight; 
				activeGraph.setEdgeWeight(sourceNode, targetNode, newWeight);
				statusBar()->showMessage( QString(tr("Ready.")) ,statusBarDuration);
				graphChanged();
				return;
			}
			else {
				statusBar()->showMessage( QString(tr("Change link weight cancelled.")) , statusBarDuration);
				return;
			}
		}
		linkClicked=FALSE;
	}
	
}




/**
*	Filters Nodes by their value   
	TODO slotFilterNodes
*	
*/
void MainWindow::slotFilterNodes(){

	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Nothing to filter! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

		statusBar()->showMessage( QString(tr("Nothing to filter!")) , statusBarDuration);
		return;
	}
}




/**
*	Filters links according to their weight 
*	All links weighted more (or less) than the specified w will be removed.
*/ 

void MainWindow::slotFilterLinks(){
/*	mapEdges.clear();
	if (!fileLoaded && !networkModified  )   {
		statusBar()->showMessage( QString(tr("Load a network file first. \nThen you may ask me to compute something!")) , statusBarDuration);
		return;
	}
	bool ok=FALSE, moreWeighted=FALSE;
	int selectedWeight=0;
	switch (
             QMessageBox::information(this, "Filter Links",tr(" Do you want to filter out links weighted greater-equal or less-equal than a number?"), ">=","<=",0,1)
            )
	{
		case 0:  moreWeighted = TRUE;
			selectedWeight =   QInputDialog::getInteger(
			"Filter Links", tr("Filter all links with weight greater-equal than: "),
			0, 0, 10, 1, &ok, this ) ;
			break;
		case 1:  moreWeighted=FALSE;
			selectedWeight =   QInputDialog::getInteger(
			"Filter Links", tr("Filter all links with weight less-equal than: "),
			0, 0, 10, 1, &ok, this ) ;
			break;
	}
	QList<QGraphicsItem *> list=scene->items();
	for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
		if ( (*it) -> type() == Link_Rtti ){
			Edge *link = (Edge*) (*it);
			if ( link->weight()>=selectedWeight && moreWeighted){
				(*it)->setVisible(false);
				mapEdges [ link->sourceNodeNumber() ] = link->targetNodeNumber();
				qDebug("GT: Hiding link from node %i to node %i",link->sourceNodeNumber(),mapEdges[link->sourceNodeNumber() ]);

			}
			else if ( link->weight()<=selectedWeight && !moreWeighted) {
				(*it)->setVisible(false);
				mapEdges [ link->sourceNodeNumber() ] = link->targetNodeNumber();
				qDebug("LT: Hiding link from node %i to node %i",link->sourceNodeNumber(),mapEdges[link->sourceNodeNumber() ]);
			}
			else {
// 				mapEdges [link->sourceNodeNumber() ]= -1; 
				(*it)->setVisible(true);
			}
		}
	for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
		if ( (*it) -> type() == Arrow_Rtti ){
			Arrow *arrow = (Arrow*) (*it);
			qDebug("Arrow from node %i to node %i",arrow->fromNode(),mapEdges[arrow->sourceNodeNumber() ]);
			if ( mapEdges[arrow->sourceNodeNumber() ]  ){
				qDebug(" Hiding arrow from node %i to node %i",arrow->sourceNodeNumber(),mapEdges[arrow->sourceNodeNumber() ]);
				arrow->setVisible(false);
			}
			else (*it)->setVisible(true);
		}

	statusBar()->showMessage (QString(tr("Ready")), statusBarDuration) ;*/
}




/**
*	Transforms all nodes to links
	TODO slotTransformNodes2Links
*/
void MainWindow::slotTransformNodes2Links(){
	graphChanged();

}





/**
*	Converts all edges to double edges, so that the network becomes undirected (symmetric adjacency matrix).
*/
void MainWindow::slotSymmetrize(){
	if (!fileLoaded && !networkModified )  {
		QMessageBox::critical(this, "Error",tr("No links here! \nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("No links present...") ,statusBarDuration);
		return;
	}
	qDebug("MW: slotSymmetrize() calling symmetrize");
	activeGraph.symmetrize();
	QMessageBox::information(this, "Symmetrize",tr("All links are reciprocal. \nYour network is symmetric..."), "OK",0);
	statusBar()->showMessage (QString(tr("Ready")), statusBarDuration) ;
}



/**
	TODO slotColorationStrongStructural
*/
void MainWindow::slotColorationStrongStructural() {
}


/**
	TODO slotColorationRegular
*/
void MainWindow::slotColorationRegular() {
}


/**
	TODO slotLayoutRandom
*/
void MainWindow::slotLayoutRandom(){

}


/** 
	TODO slotLayoutRandomCircle
*/
void MainWindow::slotLayoutRandomCircle(){
}





/**
	slotLayoutSpringEmbedder called from menu
*/
void MainWindow::slotLayoutSpringEmbedder(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("There are node nodes yet!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
		statusBar()->showMessage(tr("I am really sorry. You must really load a file first... ") ,statusBarDuration);
		return;
	}
	if (moveSpringEmbedderBx->checkState() == Qt::Unchecked){
		statusBar()->showMessage(tr("Embedding a spring-gravitational model on the network.... ") ,statusBarDuration);
		moveSpringEmbedderBx->setCheckState(Qt::Checked);
		statusBar()->showMessage(tr("Click on the checkbox \"Spring-Embedder\" to stop movement!"), statusBarDuration);
	}
	else { 
		moveSpringEmbedderBx->setCheckState(Qt::Unchecked);
		statusBar()->showMessage(tr("Movement stopped!"), statusBarDuration);
	}
	
}



/** 
	Called from moveSpringEmbedderBx button. 
	Calls GraphicsWidget::startNodeMovement to embed a spring Embedder layout...
*/
void MainWindow::layoutSpringEmbedder (int state){
	qDebug("MW: layoutSpringEmbedder ()");
	scene->setItemIndexMethod (QGraphicsScene::NoIndex); //best when moving items
	moveFruchtermanBx->setChecked(false);
	//graphicsWidget->nodeMovement(state, 1);
	activeGraph.nodeMovement(state, 1, graphicsWidget->width(), graphicsWidget->height());
	scene->setItemIndexMethod (QGraphicsScene::BspTreeIndex); //best when not moving items
}




/**
	slotLayoutFruchterman called from menu
*/
void MainWindow::slotLayoutFruchterman(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("There are no nodes yet!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
		statusBar()->showMessage(tr("I am really sorry. You must really load a file first... ") ,statusBarDuration);
		return;
	}
	if (moveFruchtermanBx->checkState() == Qt::Unchecked){
		statusBar()->showMessage(tr("Embedding a repelling-attracting forces model on the network.... ") ,statusBarDuration);
		moveFruchtermanBx->setCheckState(Qt::Checked);
		statusBar()->showMessage(tr("Click on the checkbox \"Fruchterman-Reingold\" to stop movement!"), statusBarDuration);
	}
	else { 
		moveFruchtermanBx->setCheckState(Qt::Unchecked);
		statusBar()->showMessage(tr("Movement stopped!"), statusBarDuration);
	}
	
}


/** 
	Called from moveSpringEmbedderBx button. 
	Calls GraphicsWidget::startNodeMovement to embed a repelling-attracting forces layout...
*/
void MainWindow::layoutFruchterman (int state){
	qDebug("MW: layoutFruchterman ()");
	moveSpringEmbedderBx->setChecked(false);
	scene->setItemIndexMethod (QGraphicsScene::NoIndex); //best when moving items

	activeGraph.nodeMovement(state, 2, graphicsWidget->width(), graphicsWidget->height());
// graphicsWidget->nodeMovement(state, 2);
	scene->setItemIndexMethod (QGraphicsScene::BspTreeIndex); //best when not moving items
}


/** 
	Resizes all nodes according to the amount of their out-Links from other nodes.
*/
void MainWindow::slotLayoutNodeSizeProportionalOutEdges(bool checked){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Wake up! \nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
		statusBar()->showMessage(tr("I am really sorry. You must really load a file first... ") ,statusBarDuration);
		return;
	}

	qDebug("MW: slotLayoutNodeSizeProportionalOutEdges()");
	QList<QGraphicsItem *> list=scene->items();
	int edges = 0, size = initNodeSize ;
	
	if (checked != true) {
		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
		for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)  {
			if ( (*it) -> type() == TypeNode ){
				Node *jim = (Node*) (*it);
				(*jim).setSize(size);
			}
		}
		
		nodeSizeProportionalOutDegreeAct->setChecked(false);
		nodeSizeProportional2OutDegreeBx->setChecked(false);
		QApplication::restoreOverrideCursor();
		return;
	}
	nodeSizeProportionalOutDegreeAct->setChecked(true);
	nodeSizeProportional2OutDegreeBx->setChecked(true);
	statusBar()->showMessage(tr("Embedding node size model on the network.... ") ,statusBarDuration);	
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++) {
		if ( (*it) -> type() == TypeNode ){
			Node *jim = (Node*) (*it);
			edges = activeGraph.edgesFrom(  (*jim).nodeNumber() ) ;
			qDebug("Node %i outDegree: %i ", (*jim).nodeNumber(), edges);
			
			if (edges == 0 ) {
				size = initNodeSize; 
			}
			else if (edges == 1 ) {
				size = initNodeSize + 1; 
			}
			else if (edges > 1 && edges <= 2  ) {
				size = initNodeSize + 2; 	
			}
			else if (edges > 2 && edges <= 3  ) {
				size = initNodeSize + 3; 
			}
			else if (edges > 3 && edges <= 4  ) {
				size = initNodeSize + 4; 
			}
			else if (edges > 4 && edges <= 6  ) {
				size = initNodeSize + 4; 
			}
			else if (edges > 5 && edges <= 7  ) {
				size = initNodeSize+5 ; 
			}
			else if (edges > 7 && edges <= 10  ) {
				size = initNodeSize+6 ; 
			}
			else if (edges > 10 && edges <= 15  ) {
				size = initNodeSize+7 ; 
			}
			else if (edges > 15 && edges <= 25  ) {
				size = initNodeSize+8 ; 
			}
			else  if (edges > 25 ) {
				size = initNodeSize+9; 
			}
			qDebug("Changing size of %i to %i", (*jim).nodeNumber(), size);
			(*jim).setSize(size);
		}

	}
	QApplication::restoreOverrideCursor( );

}







/** 
	Resizes all nodes according to the amount of their in-Links from other nodes.
*/
void MainWindow::slotLayoutNodeSizeProportionalInEdges(bool checked){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("You must be dreaming! \nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
		statusBar()->showMessage(tr("I am really sorry. You must really load a file first... ") ,statusBarDuration);
		return;
	}

	qDebug("MW: slotLayoutNodeSizeProportionalInEdges()");
	QList<QGraphicsItem *> list=scene->items();
	int edges = 0, size = initNodeSize ;
	
	if (checked != true) {
		for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)  {
			if ( (*it) -> type() == TypeNode ){
				Node *jim = (Node*) (*it);
				(*jim).setSize(size);
			}
		}
		nodeSizeProportionalInDegreeAct->setChecked(false);
		nodeSizeProportional2InDegreeBx->setChecked(false);
		return;
	}
	nodeSizeProportionalInDegreeAct->setChecked(true);
	nodeSizeProportional2InDegreeBx->setChecked(true);
	statusBar()->showMessage(tr("Embedding node size model on the network.... ") ,statusBarDuration);	
	for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++) {
		if ( (*it) -> type() == TypeNode ){
			Node *jim = (Node*) (*it);
			edges = activeGraph.edgesTo(  (*jim).nodeNumber() ) ;
			qDebug("Node %i inDegree: %i ", (*jim).nodeNumber(), edges);
			
			if (edges == 0 ) {
				size = initNodeSize; 
			}
			else if (edges == 1 ) {
				size = initNodeSize + 1; 
			}
			else if (edges > 1 && edges <= 2  ) {
				size = initNodeSize + 2; 	
			}
			else if (edges > 2 && edges <= 3  ) {
				size = initNodeSize + 3; 
			}
			else if (edges > 3 && edges <= 4  ) {
				size = initNodeSize + 4; 
			}
			else if (edges > 4 && edges <= 6  ) {
				size = initNodeSize + 4; 
			}
			else if (edges > 5 && edges <= 7  ) {
				size = initNodeSize+5 ; 
			}
			else if (edges > 7 && edges <= 10  ) {
				size = initNodeSize+6 ; 
			}
			else if (edges > 10 && edges <= 15  ) {
				size = initNodeSize+7 ; 
			}
			else if (edges > 15 && edges <= 25  ) {
				size = initNodeSize+8 ; 
			}
			else  if (edges > 25 ) {
				size = initNodeSize+9; 
			}
			qDebug("Changing size of %i to %i", (*jim).nodeNumber(), size);
			(*jim).setSize(size);
		}

	}

}




/**
*	Calls Graph::layoutCircleCentrality()
*	to reposition all nodes on a circular layout based on their In-Degree Centralities. 
*	More central nodes are closer to the centre
*/
void MainWindow::slotLayoutCircleCentralityInDegree(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Sorry, I can't follow! \nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
		statusBar()->showMessage( QString(tr("Nothing to layout! Are you dreaming?")) , statusBarDuration);
		return;
	}
	double x0=scene->width()/2.0;
	double y0=scene->height()/2.0;
	double maxRadius=(graphicsWidget->height()/2.0)-50;          //pixels
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	statusBar()->showMessage( QString(tr("Calculating new nodes positions. Please wait...")) , 10*statusBarDuration);
	graphicsWidget->clearBackgrCircles();
	activeGraph.layoutCircleCentrality(x0, y0, maxRadius,1);
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(tr("Nodes in inner circles have greater In-Degree Centrality."),statusBarDuration);	
}


/**
*	Calls Graph::layoutCircleCentrality()
*	to reposition all nodes on a circular layout based on their Out-Degree Centralities. 
*	More central nodes are closer to the centre
*/
void MainWindow::slotLayoutCircleCentralityOutDegree(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Load a network file or create a new network first!"), "OK",0);
		statusBar()->showMessage( QString(tr("Nothing to layout! Are you dreaming?")) , statusBarDuration);
		return;
	}
	double x0=scene->width()/2.0;
	double y0=scene->height()/2.0;
	double maxRadius=(graphicsWidget->height()/2.0)-50;          //pixels
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	statusBar()->showMessage( QString(tr("Calculating new nodes positions. Please wait...")) , 10*statusBarDuration);
	graphicsWidget->clearBackgrCircles();
	activeGraph.layoutCircleCentrality(x0, y0, maxRadius, 2);
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(tr("Nodes in inner circles have greater Out-Degree Centrality. "),statusBarDuration);	
}




/**
*	Calls Graph::layoutCircleCentrality()
*	to reposition all nodes on a circular layout based on their Closeness Centralities. 
*	More central nodes are closer to the centre
*/
void MainWindow::slotLayoutCircleCentralityCloseness(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Sorry, there are no nodes yet!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);

		statusBar()->showMessage( QString(tr("Nothing to layout! Are you dreaming?")) , statusBarDuration);
		return;
	}
	double x0=scene->width()/2.0;
	double y0=scene->height()/2.0;
	double maxRadius=(graphicsWidget->height()/2.0)-50;          //pixels
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	statusBar()->showMessage( QString(tr("Calculating new nodes positions. Please wait...")) , 10*statusBarDuration);
	graphicsWidget->clearBackgrCircles();
	activeGraph.layoutCircleCentrality(x0, y0, maxRadius,3);
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(tr("Nodes in inner circles have greater Closeness Centrality. "),statusBarDuration);	
}





/**
*	Calls Graph::layoutCircleCentrality()
*	to reposition all nodes on a circular layout based on their Betweeness Centralities. 
*	More central nodes are closer to the centre
*/
void MainWindow::slotLayoutCircleCentralityBetweeness(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("No nodes yet!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);

		statusBar()->showMessage( QString(tr("Nothing to layout! Are you dreaming?")) , statusBarDuration);
		return;
	}
	double x0=scene->width()/2.0;
	double y0=scene->height()/2.0;
	double maxRadius=(graphicsWidget->height()/2.0)-50;          //pixels
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	statusBar()->showMessage( QString(tr("Calculating new nodes positions. Please wait...")) , 10*statusBarDuration);
	graphicsWidget->clearBackgrCircles();
	activeGraph.layoutCircleCentrality(x0, y0, maxRadius,4);
	QApplication::restoreOverrideCursor();

	statusBar()->showMessage(tr("Nodes in inner circles have greater Betweeness Centrality. "),statusBarDuration);	
}






/**
*	Calls Graph::layoutCircleCentrality()
*	to reposition all nodes on a circular layout based on their Stress Centralities. 
*	More central nodes are closer to the centre
*/
void MainWindow::slotLayoutCircleCentralityStress(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Nothing to do!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
		statusBar()->showMessage( QString(tr("Nothing to layout! Are you dreaming?")) , statusBarDuration);
		return;
	}
	double x0=scene->width()/2.0;
	double y0=scene->height()/2.0;
	double maxRadius=(graphicsWidget->height()/2.0)-50;          //pixels
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	statusBar()->showMessage( QString(tr("Calculating new nodes positions. Please wait...")) , 10*statusBarDuration);
	graphicsWidget->clearBackgrCircles();
	activeGraph.layoutCircleCentrality(x0, y0, maxRadius,5);
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(tr("Nodes in inner circles have greater Stress Centrality. "),statusBarDuration);	
}




/**
*	Calls Graph::layoutCircleCentrality()
*	to reposition all nodes on a circular layout based on their Graph Centralities. 
*	More central nodes are closer to the centre
*/
void MainWindow::slotLayoutCircleCentralityGraph(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Nothing to do here!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
		statusBar()->showMessage( QString(tr("Nothing to layout! Are you dreaming?")) , statusBarDuration);
		return;
	}
	double x0=scene->width()/2.0;
	double y0=scene->height()/2.0;
	double maxRadius=(graphicsWidget->height()/2.0)-50;          //pixels
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	statusBar()->showMessage( QString(tr("Calculating new nodes positions. Please wait...")) , 10*statusBarDuration);
	graphicsWidget->clearBackgrCircles();
	activeGraph.layoutCircleCentrality(x0, y0, maxRadius,6);
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(tr("Nodes in inner circles have greater Graph Centrality. "),statusBarDuration);	

}



/**
*	Calls Graph::layoutCircleCentrality()
*	to reposition all nodes on a circular layout based on their Eccentricities 
*	More central nodes are closer to the centre
*/
void MainWindow::slotLayoutCircleCentralityEccentr(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Nothing to do!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
		statusBar()->showMessage( QString(tr("Nothing to layout! Are you dreaming?")) , statusBarDuration);
		return;
	}
	double x0=scene->width()/2.0;
	double y0=scene->height()/2.0;
	double maxRadius=(graphicsWidget->height()/2.0)-50;          //pixels
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	statusBar()->showMessage( QString(tr("Calculating new nodes positions. Please wait...")) , 10*statusBarDuration);
	graphicsWidget->clearBackgrCircles();
	activeGraph.layoutCircleCentrality(x0, y0, maxRadius,7);
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(tr("Nodes in inner circles have greater Eccentricity Centrality. "),statusBarDuration);	

}

/**
*	Calls Graph::layoutCircleCentrality()
*	to reposition all nodes on a circular layout based on their Informational Centralities. 
*	More central nodes are closer to the centre
*/
void MainWindow::slotLayoutCircleCentralityInformational(){
}



/**
*	Calls Graph::layoutLevelCentrality 
*	to reposition all nodes on different top-down levels according to their 
*	In-Degree Centrality
*	More central nodes are closer to the top of the canvas
*/
void MainWindow::slotLayoutLevelCentralityInDegree(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Nothing to do!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
		statusBar()->showMessage( QString(tr("Nothing to layout! Are you dreaming?")) , statusBarDuration);
		return;
	}
	double maxWidth=scene->width();
	double maxHeight=scene->height(); //pixels
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	statusBar()->showMessage( QString(tr("Calculating new nodes positions. Please wait...")) , 10*statusBarDuration);
	graphicsWidget->clearBackgrCircles();
	activeGraph.layoutLevelCentrality(maxWidth, maxHeight, 1);
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(tr("Nodes in upper levels have greater In-Degree Centrality. "),statusBarDuration);	

}



/**
*	Calls Graph::layoutLevelCentrality 
*	to reposition all nodes on different top-down levels according to their 
*	Out-Degree Centrality
*	More central nodes are closer to the top of the canvas
*/
void MainWindow::slotLayoutLevelCentralityOutDegree(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Load a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
		statusBar()->showMessage( QString(tr("Nothing to layout! Are you dreaming?")) , statusBarDuration);
		return;
	}
	double maxWidth=scene->width();
	double maxHeight=scene->height(); //pixels
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	statusBar()->showMessage( QString(tr("Calculating new nodes positions. Please wait...")) , 10*statusBarDuration);
	graphicsWidget->clearBackgrCircles();
	activeGraph.layoutLevelCentrality(maxWidth, maxHeight, 2);
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(tr("Nodes in upper levels have greater Out-Degree Centrality. "),statusBarDuration);	

}


/**
*	Calls Graph::layoutLevelCentrality 
*	to reposition all nodes on different top-down levels according to their 
*	Closeness Centrality
*	More central nodes are closer to the top of the canvas

*/
void MainWindow::slotLayoutLevelCentralityCloseness(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Load a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
		statusBar()->showMessage( QString(tr("Nothing to layout! Are you dreaming?")) , statusBarDuration);
		return;
	}
	double maxWidth=scene->width();
	double maxHeight=scene->height(); //pixels
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	statusBar()->showMessage( QString(tr("Calculating new nodes positions. Please wait...")) , statusBarDuration);
	graphicsWidget->clearBackgrCircles();
	activeGraph.layoutLevelCentrality(maxWidth, maxHeight, 3);
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(tr("Nodes in upper levels have greater Closeness Centrality."),statusBarDuration);	
}



/**
*	Calls Graph::layoutLevelCentrality 
*	to reposition all nodes on different top-down levels according to their 
*	Betweeness Centrality
*	More central nodes are closer to the top of the canvas
*/
void MainWindow::slotLayoutLevelCentralityBetweeness(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Nothing to do!\nLoad a network file or create a new network first. \nThen we can talk about layouts!"), "OK",0);
		statusBar()->showMessage( QString(tr("Nothing to layout! Are you dreaming?")) , statusBarDuration);
		return;
	}
	double maxWidth=scene->width();
	double maxHeight=scene->height(); //pixels
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	statusBar()->showMessage( QString(tr("Calculating new nodes positions. Please wait...")) , 10*statusBarDuration);
	graphicsWidget->clearBackgrCircles();
	activeGraph.layoutLevelCentrality(maxWidth, maxHeight, 4);
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(tr("Nodes in upper levels have greater Betweeness Centrality. "),statusBarDuration);	
}


/**
	TODO slotLayoutLevelCentralityInformational
*/
void MainWindow::slotLayoutLevelCentralityInformational(){
}







/**
*	Returns the amount of active links on the scene.
*/
int MainWindow::activeLinks(){
	qDebug ("activeLinks()");
	totalLinks=activeGraph.totalEdges();
	return totalLinks;
}








/**
*	Returns the amount of active nodes on the scene.
*/
int MainWindow::activeNodes(){ 
	return activeGraph.vertices();
}







/**
*	Displays a box informing the user about the symmetry or not of the adjacency matrix
*/

void MainWindow::slotCheckSymmetry(){
		if (!fileLoaded && !networkModified  )   {
		QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
		statusBar()->showMessage( QString(tr("There is no network!")) , statusBarDuration);
		return;
	}
	if (activeGraph.isSymmetric())
		QMessageBox::information(this, "Network Symmetry", tr("Adjacency matrix symmetry = YES "),"OK",0);
	else
		QMessageBox::information(this, "Network Symmetry", tr("Adjacency matrix symmetry = NO "),"OK",0);
	
	statusBar()->showMessage (QString(tr("Ready")), statusBarDuration) ;

}


/**
*	Calculates and displays the density of the network.
*/
void MainWindow::slotNetworkDensity() {
// 	if (!fileLoaded && !networkModified  )   {
// 		QMessageBox::critical(this, "Error",tr("Load a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
// 		statusBar()->showMessage( QString(tr("No network created or loaded yet!")) , statusBarDuration);
// 		return;
// 	}
// 	if ( activeGraph.totalEdges() ==-1 ){
// 		statusBar()->showMessage (QString(tr("ERROR in Links count")), statusBarDuration) ;
// 		return;
// 	}
// 	if ( activeGraph.vertices() ==-1 ){
// 		statusBar()->showMessage (QString(tr("ERROR in nodes count")), statusBarDuration) ;
// 		return;
// 	}
// 	
// 	QMessageBox::information(this, "Density", tr("Network density = ")+QString::number(activeGraph.density()),"OK",0);
// 	statusBar()->showMessage (QString(tr("Ready")), statusBarDuration) ;
}





/**
*  Displays the graph distance (geodesic distance) between two user-specified nodes
	This is the length of the shortest path between them.
*/
void MainWindow::slotGraphDistance(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
		statusBar()->showMessage( QString(tr("There are no nodes. Nothing to do...")) , statusBarDuration);
		return;
	}
	bool ok=FALSE;
	int  min=1, max=1, i=-1, j=-1;
	QList<QGraphicsItem *> list=scene->items();
	for (QList<QGraphicsItem *> ::iterator it=list.begin(); it!=list.end(); it++)
		if ( (*it) -> type() == TypeNode ){
			Node *jim = (Node*) (*it);
			if (min>jim->nodeNumber() ) min=jim->nodeNumber();
			if (max<jim->nodeNumber() ) max=jim->nodeNumber();
		}
	i=QInputDialog::getInteger(this, tr("Distance between two nodes"),tr("Select source node:  ("+QString::number(min).toAscii()+"..."+QString::number(max).toAscii()+"):"), min, 1, max , 1, &ok )   ;
	if (!ok) {
		statusBar()->showMessage("Distance calculation operation cancelled.", statusBarDuration);
		return;
	}

	j=QInputDialog::getInteger(this, tr("Distance between two nodes"), tr("Select target node:  ("+QString::number(min).toAscii()+"..."+QString::number(max).toAscii()+"):"),min, 1, max , 1, &ok )   ;
	if (!ok) {
		statusBar()->showMessage(tr("Distance calculation operation cancelled."), statusBarDuration);
		return;
	}

	qDebug("source %i target %i",i, j);
		
	if (activeGraph.isSymmetric() && i>j) {
		qSwap(i,j);
	}
	if ( activeGraph.distance(i,j) != 0 )
		QMessageBox::information(this, tr("Distance"), tr("Network distance (")+QString::number(i)+", "+QString::number(j)+") = "+QString::number(activeGraph.distance(i,j))+tr("\nThe nodes are connected."),"OK",0);
	else 
		QMessageBox::information(this, tr("Distance"), tr("Network distance (")+QString::number(i)+", "+QString::number(j)+") = "+QString::number(activeGraph.distance(i,j))+tr("\nThe nodes are not connected."),"OK",0);
}




/**
*  Invokes creation of the matrix of nodes' distances, then displays it.
*/
void MainWindow::slotViewDistanceMatrix(){
	qDebug("MW: slotViewDistanceMatrix()");
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("There are no nodes nor links!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
		statusBar()->showMessage( QString(tr("Nothing to do!")) , statusBarDuration);
		return;
	}

	statusBar()->showMessage(tr("Creating distance matrix. Please wait..."), statusBarDuration+2000);	
	char fn[]= "distance-matrix.dat";
	char fn1[]="sigmas-matrix.dat";
	activeGraph.writeDistanceMatrix(fn, fn1, networkName.toLocal8Bit());
	//Open a text editor window for the new file created by graph class
	QString qfn1=QString::fromLocal8Bit(fn1);
	TextEditor *ed = new TextEditor(fn);
	TextEditor *ed1 = new TextEditor(fn1);

	ed1->setWindowTitle(tr("Matrix of sigmas "));
	ed->setWindowTitle(tr("Matrix of distances "));
	ed1->show();	
	ed->show();


	
}




/**  Displays the network diameter (largest geodesic) */
void MainWindow::slotDiameter() {
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("There are no nodes nor links!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

		statusBar()->showMessage( QString(tr("Cannot find the diameter of nothing...")) , statusBarDuration);
		return;
	}
	activeGraph.createDistanceMatrix(false);
	int netDiameter=activeGraph.diameter();
	QApplication::restoreOverrideCursor();	

	if (netDiameter > (activeGraph.vertices()-1) ) 
		QMessageBox::information(this, "Diameter", "Network diameter = "+ QString::number(netDiameter)+"  > (vertices()-1).", "OK",0);
	else 
		QMessageBox::information(this, "Diameter", "Network diameter = " + QString::number(netDiameter), "OK",0);
	statusBar()->showMessage(tr("Diameter calculated. Ready."), statusBarDuration);

}





/**  Displays the  average shortest path length (average graph distance) */
void MainWindow::slotAverageGraphDistance() {
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("There are no nodes nor links!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

		statusBar()->showMessage( QString(tr("Cannot find the diameter of nothing...")) , statusBarDuration);
		return;
	}
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	float averGraphDistance=activeGraph.averageGraphDistance();
	QApplication::restoreOverrideCursor();	


	QMessageBox::information(this, "Average Graph Distance", "The average shortest path length is  = " + QString::number(averGraphDistance), "OK",0);
	statusBar()->showMessage(tr("Average distance calculated. Ready."), statusBarDuration);

}






/**
*	Displays a message	on the status bar when you resize the window.
*/
void MainWindow::windowInfoStatusBar(int w, int h){
	statusBar()->showMessage( QString(tr("Window resized to (%1, %2) pixels.")).arg(w).arg(h), statusBarDuration);
}



/**
*	Writes Clustering Coefficients into a file, then displays it.
*/
void MainWindow::slotClusteringCoefficient (){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Nothing to do! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
		statusBar()->showMessage( QString(tr(" No network here. Sorry. Nothing to do.")) , statusBarDuration);
		return;
	}
	float clucof=0;
	
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	clucof=activeGraph.clusteringCoefficient();
	QApplication::restoreOverrideCursor();
	
	QString fn = "clustering-coefficients.dat";
	QFile file( fn );
	if ( !file.open( QIODevice::WriteOnly ) )
		return;
	QTextStream ts(&file  );


	ts <<"-SocNetV- "<<VERSION<<"\n\n";
	ts <<tr("CLUSTERING COEFFICIENT REPORT \n");
	ts <<tr("Created: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
	ts<< tr("CLUSTERING COEFFICIENT (CLC) OF EACH NODE\n");
	ts<< tr("CLC  range: 0 < C < 1") <<"\n";
	ts << "Node"<<"\tCLC\n";

	
	QList<Vertex*>::iterator it;
	for (it=activeGraph.m_graph.begin(); it!=activeGraph.m_graph.end(); it++){ 
		ts<<(*it)->name()<<"\t"<<(*it)->CLC() <<endl;
	}
	if (activeGraph.isSymmetric()) {
		ts<< "\nAverage Clustering Coefficient = "<< activeGraph.averageCLC<<"\n" ;
	//	ts<< "DC Variance = "<< activeGraph.varianceDegree<<"\n\n";
	}
	else{
		ts<< "\nAverage Clustering Coefficient= "<< activeGraph.averageCLC<<"\n" ;
//		ts<< "ODC Variance = "<< activeGraph.varianceDegree<<"\n\n";
	}
	if ( activeGraph.minCLC == activeGraph.maxCLC )
		ts<< "\nAll nodes have the same clustering coefficient value.\n";
	else  {
		ts<< "\nNode "<< activeGraph.maxNodeCLC << " has the maximum Clustering Coefficient: " << activeGraph.maxCLC <<"  \n";
		ts<< "\nNode "<< activeGraph.minNodeCLC << " has the minimum Clustering Coefficient: " << activeGraph.minCLC <<"  \n";
	}

	
	ts<<"\nGRAPH CLUSTERING COEFFICIENT (GCLC)\n\n";
	ts<<"GCLC = " << activeGraph.averageCLC<<"\n\n";
	ts<<tr("Range: 0 < GCLC < 1\n");
	ts<<tr("GCLC = 0, when there are no cliques (i.e. acyclic tree).\n");
	ts<<tr("GCLC = 1, when every node and its neighborhood are complete cliques.\n");

	file.close();

	TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
	tempFileNameNoPath=fn.split( "/");
	ed->setWindowTitle("Clustering Coefficients saved as: " + tempFileNameNoPath.last());
	ed->show();
	
}



/**
*	Writes Out-Degree Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityOutDegree(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Nothing to do! \nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
		statusBar()->showMessage( QString(tr(" No network here. Sorry. Nothing to do.")) , statusBarDuration);
		return;
	}
	bool considerWeights=FALSE;
	
	switch( QMessageBox::information( this, "Centrality Out-Degree",
				      tr("Take weights into account (Default: No)?"),
				      tr("Yes"), tr("No"),
				      0, 1 ) )
	{
		case 0:
			considerWeights=TRUE;
			break;
		case 1:
			considerWeights=FALSE;
			break;
		default: // just for sanity
			considerWeights=TRUE;
			return;
		break;
	}

	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	activeGraph.centralityOutDegree(considerWeights);
	QApplication::restoreOverrideCursor();
	
	
	QString fn = "centrality-out-degree.dat";
	QFile file( fn );
	if ( !file.open( QIODevice::WriteOnly ) )
		return;
	QTextStream ts(&file  );

	float maximumIndexValue=activeGraph.vertices()-1.0;
	ts <<"-SocNetV- "<<VERSION<<"\n\n";
	ts <<tr("OUT-DEGREE CENTRALITY REPORT \n");
	ts <<tr("Created: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
	ts<< tr("OUT-DEGREE CENTRALITIES (ODC) OF EACH NODE\n");
	ts<< tr("ODC  range: 0 < C < ")<<QString::number(maximumIndexValue)<<"\n";
	ts<< tr("ODC' range: 0 < C'< 1")<<"\n\n";
	ts << "Node"<<"\tODC\tODC'\t\t%ODC\n";

	
	QList<Vertex*>::iterator it;
	for (it=activeGraph.m_graph.begin(); it!=activeGraph.m_graph.end(); it++){ 
		ts<<(*it)->name()<<"\t"<<(*it)->ODC() << "\t"<< (*it)->SODC() << "\t\t" <<  (100* ((*it)->ODC()) / activeGraph.sumODC)<<endl;
	}
	if (activeGraph.isSymmetric()) {
		ts<< "Mean DC= "<< activeGraph.meanDegree<<"\n" ;
		ts<< "DC Variance = "<< activeGraph.varianceDegree<<"\n\n";
	}
	else{
		ts<< "Mean ODC= "<< activeGraph.meanDegree<<"\n" ;
		ts<< "ODC Variance = "<< activeGraph.varianceDegree<<"\n\n";
	}
	if ( activeGraph.minODC == activeGraph.maxODC )
		ts<< "\nAll nodes have the same ODC value.\n";
	else  {
		ts<< "\nNode "<< activeGraph.maxNodeODC << " has the maximum ODC value (std): " << activeGraph.maxODC <<"  \n";
		ts<< "\nNode "<< activeGraph.minNodeODC << " has the minimum ODC value (std): " << activeGraph.minODC <<"  \n";
	}
	if (activeGraph.classesODC!=1)
		ts<< "\nThere are "<<activeGraph.classesODC<<" different ODC classes.\n";	
	else 
		ts<< "\nThere is only "<<activeGraph.classesODC<<" ODC class.\n";	

	
	ts<<"\nGROUP OUT-DEGREE CENTRALISATION (GODC)\n\n";
	ts<<"GODC = " << activeGraph.groupODC<<"\n\n";
	ts<<tr("GODC range: 0 < GODC < 1\n");
	ts<<tr("GODC = 0, when all in-degrees are equal (i.e. regular lattice).\n");
	ts<<tr("GODC = 1, when one node completely dominates or overshadows the other nodes.\n");
	ts<<"(Wasserman & Faust, formula 5.5, p. 177)\n\n";
	ts<<tr("The degree of the node is a measure of the \'activity\' of the node it represents\n");
	ts<<"(Wasserman & Faust, p. 101)\n";

	file.close();

	TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
	tempFileNameNoPath=fn.split( "/");
	ed->setWindowTitle("Out-Degree Centralities saved as: " + tempFileNameNoPath.last());
	ed->show();
}




/**
*	Writes In-Degree Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityInDegree(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("Nothing to do!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);
		statusBar()->showMessage( QString(tr("Nothing to do...")) , statusBarDuration);
		return;
	}
	bool considerWeights=FALSE;
	
	switch( QMessageBox::information( this, "Centrality In-Degree",
				      tr("Take weights into account (Default: No)?"),
				      tr("Yes"), tr("No"),
				      0, 1 ) )
	{
		case 0:
			considerWeights=TRUE;
			break;
		case 1:
			considerWeights=FALSE;
			break;
		default: // just for sanity
			considerWeights=TRUE;
			return;
		break;
	}

	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

	activeGraph.centralityInDegree(considerWeights);
	
	QApplication::restoreOverrideCursor();
			
	QString fn = "centrality-in-degree.dat";
	QFile file( fn );
	if ( !file.open( QIODevice::WriteOnly ) )
		return;
	QTextStream ts(&file  );

	float maximumIndexValue=activeGraph.vertices()-1.0;
	ts <<"-SocNetV- "<<VERSION<<"\n\n";
	ts <<tr("IN-DEGREE CENTRALITY REPORT \n");
	ts <<tr("Created: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
	ts<< tr("IN-DEGREE CENTRALITIES (IDC) OF EACH NODE\n");
	ts<< tr("IDC  range: 0 < C < ")<<QString::number(maximumIndexValue)<<"\n";
	ts<< tr("IDC' range: 0 < C'< 1")<<"\n\n";
	ts << "Node"<<"\tIDC\tIDC'\t\t%IDC\n";

	QList<Vertex*>::iterator it;
	for (it=activeGraph.m_graph.begin(); it!=activeGraph.m_graph.end(); it++){ 
		ts<<(*it)->name()<<"\t"<<(*it)->IDC() << "\t"<< (*it)->SIDC() << "\t\t" <<  (100* ((*it)->IDC()) / activeGraph.sumIDC)<<endl;
	}
	if (activeGraph.isSymmetric()) {
		ts<< "Mean DC = "<< activeGraph.meanDegree<<"\n" ;
		ts<< "DC Variance = "<< activeGraph.varianceDegree<<"\n\n";
	}
	else{
		ts<< "Mean IDC = "<< activeGraph.meanDegree<<"\n" ;
		ts<< "IDC Variance = "<< activeGraph.varianceDegree<<"\n\n";
	}
	if ( activeGraph.minIDC == activeGraph.maxIDC )
		ts<< "\nAll nodes have the same IDC value.\n";
	else  {
		ts<< "\nNode "<< activeGraph.maxNodeIDC << " has the maximum IDC value (std): " << activeGraph.maxIDC <<"  \n";
		ts<< "\nNode "<< activeGraph.minNodeIDC << " has the minimum IDC value (std): " << activeGraph.minIDC <<"  \n";
	}
	if (activeGraph.classesIDC!=1)
		ts<< "\nThere are "<<activeGraph.classesIDC<<" different IDC classes.\n";	
	else 
		ts<< "\nThere is only "<<activeGraph.classesIDC<<" IDC class.\n";	


	ts<<"\nGROUP IN-DEGREE CENTRALISATION (GIDC)\n\n";
	ts<<"GIDC = " << activeGraph.groupIDC<<"\n\n";
	ts<<tr("GIDC range: 0 < GIDC < 1\n");
	ts<<tr("GIDC = 0, when all in-degrees are equal (i.e. regular lattice).\n");
	ts<<tr("GIDC = 1, when one node is linked from every other node.\n");
	ts<<tr("The in-degree of the node is a measure of the \'activity\' of the node it represents\n");
	ts<<"(Wasserman & Faust, p. 101)\n";

	file.close();

	TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
	tempFileNameNoPath=fn.split( "/");
	ed->setWindowTitle("In-Degree Centralities saved as: " + tempFileNameNoPath.last());
	ed->show();
}




/**
*	Writes Closeness Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityCloseness(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

		statusBar()->showMessage( QString(tr("Nothing to do...")) , statusBarDuration);
		return;
	}
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	activeGraph.createDistanceMatrix(true);
//	activeGraph.centralityCloseness();
	QApplication::restoreOverrideCursor();

	//float maximumIndexValue=1.0/(activeGraph.vertices()-1.0);
	QString fn = "centrality_closeness.dat";

	QFile f( fn );
	if ( !f.open( QIODevice::WriteOnly ) )
		return;
	QTextStream ts(&f  );
	ts <<"-SocNetV- "<<VERSION<<"\n\n";
	ts <<tr("CLOSENESS - CENTRALITY REPORT \n");
	ts <<tr("Created: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
	ts<< tr("CLOSENESS CENTRALITY (CC) OF EACH NODE")<<"\n";
	ts<< tr("CC(u) is the invert sum of the distances of node u from all other nodes.")<<"\n";
	ts<< tr("CC' is the standardized CC")<<"\n";
	
	ts<< tr("CC  range:  0 < C < ")<<QString::number(activeGraph.maxIndexCC)<<"\n";
	ts<< tr("CC' range:  0 < C'< 1")<<"\n\n";
	ts << "Node"<<"\tCC\t\tCC'\t\t%CC\n";
	QList<Vertex*>::iterator it;
	for (it=activeGraph.m_graph.begin(); it!=activeGraph.m_graph.end(); it++){ 
		ts<<(*it)->name()<<"\t"<<(*it)->CC() << "\t\t"<< (*it)->SCC() << "\t\t" <<  (100* ((*it)->CC()) / activeGraph.sumCC)<<endl;
	}
	qDebug ("min %f, max %f", activeGraph.minCC, activeGraph.maxCC);
	if ( activeGraph.minCC == activeGraph.maxCC )
		ts << tr("\nAll nodes have the same CC value.\n");
	else  {
		ts << tr("\nNode ")<< activeGraph.maxNodeCC << tr(" has the maximum ACC value (std): ") <<activeGraph.maxCC  <<"  \n";
		ts << tr("\nNode ")<< activeGraph.minNodeCC << tr(" has the minimum ACC value (std): ") <<activeGraph.minCC <<"  \n";
	}
	
	ts << tr("\nThere are ")<<activeGraph.classesCC<<tr(" different Closeness Centrality classes.\n");	
	ts<<tr("\nGROUP CLOSENESS CENTRALISATION (GCC)\n\n");
	ts<< tr("GCC = ") << activeGraph.groupCC<<"\n\n";
	ts<<tr("GCC range: 0 < GCC < 1\n");
	ts<<tr("GCC = 0, when the lengths of the geodesics are all equal (i.e. a complete or a circle graph).\n");
	ts<<tr("GCC = 1, when one node has geodesics of length 1 to all the other nodes, and the other nodes have geodesics of length 2 to the remaining (N-2) nodes. This is exactly the situation realised by a star graph.\n");
	ts<<"(Wasserman & Faust, formula 5.9, p. 187)\n\n";
	ts<<tr("This measure focuses on how close a node is to all\n");
	ts<<tr("the other nodes in the set of nodes. The idea is that a node\n");
	ts<<tr("is central if it can quickly interact with all others\n");
	ts<<"(Wasserman & Faust, p. 181)\n";
	f.close();
	TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
	tempFileNameNoPath=fn.split( "/");
	ed->setWindowTitle("Closeness Centralities  saved as: " + tempFileNameNoPath.last());
	ed->show();
}




/**
*	Writes Betweeness Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityBetweeness(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

		statusBar()->showMessage( QString(tr(" Nothing to do...")) , statusBarDuration);
		return;
	}

	statusBar()->showMessage( QString(tr(" Please wait...")));
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

	activeGraph.createDistanceMatrix(true);	
	statusBar()->showMessage( QString(tr(" Finished with shortest-path distances...")));

	//float maximumIndexValue=;//vertices()-1.0)*(activeGraph.vertices()-2.0)/2.0;
	QString fn = "centrality_betweeness.dat";
	QFile f( fn );
	if ( !f.open( QIODevice::WriteOnly ) )
		return;
	QTextStream ts(&f  );
	
	ts <<"-SocNetV- "<<VERSION<<"\n\n";
	ts <<tr("BETWEENESS - CENTRALITY REPORT \n");
	ts <<tr("Created: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
	ts<< tr("BETWEENESS CENTRALITY (BC) OF EACH NODE")<<"\n";
	ts<< tr("BC of a node u is the sum of delta (s,t,u) for all s,t in V")<<"\n"; 
	ts<< tr("Delta(s,t,u) is the ratio of all geodesics between s and t which run through u.")<<"\n";
	ts<< tr("Therefore, BC(u) reflects how often the node u lies on the geodesics between the other nodes of the network")<<"\n";
	ts<< tr("BC' is the standardized BC")<<"\n";
	ts<< tr("BC  range: 0 < BC < ")<<QString::number(activeGraph.maxIndexBC)<< tr(" (Number of pairs of nodes excluding i)")<<"\n";
	ts<< tr("BC' range: 0 < BC'< 1  (C' is 1 when the node falls on all geodesics)\n\n");
	ts << "Node"<<"\tBC\t\tBC'\t\t%BC\n";
	QList<Vertex*>::iterator it;
	for (it=activeGraph.m_graph.begin(); it!=activeGraph.m_graph.end(); it++){ 
		ts<<(*it)->name()<<"\t"<<(*it)->BC() << "\t\t"<< (*it)->SBC() << "\t\t" <<  (100* ((*it)->BC()) / activeGraph.sumBC)<<endl;
	}
	if (activeGraph.minBC == activeGraph.maxBC)
		ts << tr("\nAll nodes have the same BC value.\n");
	else {
		ts<<tr("\n Node ")<<activeGraph.maxNodeBC<< tr(" has the maximum BC value: ") <<activeGraph.maxBC <<"  \n";
		ts<<tr("\n Node ")<<activeGraph.minNodeBC<< tr(" has the minimum BC value: ") <<activeGraph.minBC <<"  \n";
	}

	ts << tr("\nThere are ")<<activeGraph.classesBC<<tr(" different Betweeness Centrality classes.\n");	
	ts<<tr("\nGROUP BETWEENESS CENTRALISATION (GBC)\n\n");
	ts<< tr("GBC = ") << activeGraph.groupBC<<"\n\n";
	ts<<tr("GBC range: 0 < GBC < 1\n");
	ts<<tr("GBC = 0, when all the nodes have exactly the same betweeness index.\n");
	ts<<tr("GBC = 1, when one node falls on all other geodesics between all the remaining (N-1) nodes. This is exactly the situation realised by a star graph.\n");
	ts<<"(Wasserman & Faust, formula 5.13, p. 192)\n\n";
	f.close();
	TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
	tempFileNameNoPath=fn.split( "/");
	ed->setWindowTitle("Betweeness Centralities saved as: " + tempFileNameNoPath.last());
	ed->show();
	QApplication::restoreOverrideCursor();
}




/**
*	Writes Informational Centralities into a file, then displays it.	
	TODO slotCentralityInformational
*/
void MainWindow::slotCentralityInformational(){
}




/**
*	Writes Stress Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityStress(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

		statusBar()->showMessage( QString(tr(" Nothing to do! Why dont you try creating something first?")) , statusBarDuration);
		return;
	}
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

	activeGraph.createDistanceMatrix(true);
	//activeGraph.centralityStress();
	QApplication::restoreOverrideCursor();
	
	float maximumIndexValue=(activeGraph.vertices()-1.0)*(activeGraph.vertices()-2.0)/2.0;   //When u lies on all geodesics
	
	QString fn = "centrality_stress.dat";
	QFile f( fn );
	if ( !f.open( QIODevice::WriteOnly ) )
		return;
	QTextStream ts(&f  );
	ts <<"-SocNetV- "<<VERSION<<"\n\n";
	ts <<tr("STRESS CENTRALITY REPORT \n");
	ts <<tr("Created: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";

	ts<< tr("STRESS CENTRALITY (SC) OF EACH NODE")<<"\n";
	ts<< tr("SC(u) is the sum of sigma(s,t,u): the number of geodesics from s to t through u.")<<"\n"; 
	ts<< tr("SC(u) reflects the total number of geodesics between all other nodes which run through u")<<"\n";

	ts<< tr("SC  range: 0 < SC < ")<<QString::number(maximumIndexValue)<<"\n";
	ts<< tr("SC' range: 0 < SC'< 1  (SC'=1 when the node falls on all geodesics)\n\n");
	ts << "Node"<<"\tSC\t\tSC'\t\t%SC\n";
	QList<Vertex*>::iterator it;
	for (it=activeGraph.m_graph.begin(); it!=activeGraph.m_graph.end(); it++){ 
		ts<<(*it)->name()<<"\t"<<(*it)->SC() << "\t\t"<< (*it)->SSC() << "\t\t" <<  (100* ((*it)->SC()) / activeGraph.sumSC)<<endl;
	}
	
	if (activeGraph.minSC == activeGraph.maxSC)
		ts << tr("\nAll nodes have the same SC value.\n");
	else {
		ts<<tr("\n Node ")<<activeGraph.maxNodeSC<< tr(" has the maximum SC value: ") <<activeGraph.maxSC <<"  \n";
		ts<<tr("\n Node ")<<activeGraph.minNodeSC<< tr(" has the minimum SC value: ") <<activeGraph.minSC <<"  \n";
	}

	ts << tr("\nThere are ")<<activeGraph.classesSC<<tr(" different Stress Centrality classes.\n");	

	ts<<tr("GROUP STRESS CENTRALISATION (GSC)")<<"\n";
	ts<< tr("GSC = ") << activeGraph.groupSC<<"\n\n";
	
	ts<<tr("GSC range: 0 < GSC < 1\n");
	ts<<tr("GSC = 0, when all the nodes have exactly the same stress index.\n");
	ts<<tr("GSC = 1, when one node falls on all other geodesics between all the remaining (N-1) nodes. This is exactly the situation realised by a star graph.\n");
	f.close();
	TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
	tempFileNameNoPath=fn.split( "/");
	ed->setWindowTitle("Stress Centralities saved as: " + tempFileNameNoPath.last());
	ed->show();
}



/**
*	Writes Graph Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityGraph(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

		statusBar()->showMessage( QString(tr(" Try creating a network first. \nThen I compute whatever you want...")) , statusBarDuration);
		return;
	}
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	
 	activeGraph.createDistanceMatrix(true);
	//activeGraph.centralityGraph();
	QApplication::restoreOverrideCursor();

	float maximumIndexValue=1;
	QString fn = "centrality_graph.dat";
	QFile f( fn );
	if ( !f.open( QIODevice::WriteOnly ) )
		return;
	QTextStream ts(&f  );

	ts<<"-SocNetV- "<< VERSION<<"\n\n";
	ts<<tr("GRAPH - CENTRALITY REPORT \n" );
	ts<<tr("Created: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss"))<<"\n\n"  ;
	ts<<tr("GRAPH CENTRALITY (GC) OF EACH NODE")<<"\n";
	ts<<tr("GC  range: 0 < GC < ")<<maximumIndexValue<< " (GC=1 => distance from other nodes is max 1)\n";
	ts<<tr("GC' range: 0 < GC'< 1  (GC'=1 => directly linked with all nodes)")<<"\n\n";

	ts << "Node"<<"\tGC\t\tGC'\t\t%GC\n";
	QList<Vertex*>::iterator it;
	for (it=activeGraph.m_graph.begin(); it!=activeGraph.m_graph.end(); it++){ 
		ts<<(*it)->name()<<"\t"<<(*it)->GC() << "\t\t"<< (*it)->SGC() << "\t\t" <<  (100* ((*it)->GC()) / activeGraph.sumGC)<<endl;
	}
	
	if (activeGraph.minGC == activeGraph.maxGC)
		ts << tr("\nAll nodes have the same GC value.\n");
	else {
		ts<<tr("\n Node ")<<activeGraph.maxNodeGC<< tr(" has the maximum GC value: ") <<activeGraph.maxGC <<"  \n";
		ts<<tr("\n Node ")<<activeGraph.minNodeGC<< tr(" has the minimum GC value: ") <<activeGraph.minGC <<"  \n";
	}

	ts << tr("\nThere are ")<<activeGraph.classesGC<<tr(" different Graph Centrality classes.\n");	

	ts<<tr("\nGROUP GRAPH CENTRALISATION (GGC)\n\n");

	ts<< tr("GGC = ") << activeGraph.groupGC<<"\n\n";

	ts<<tr("GGC range: 0 < GGC < 1\n");
	ts<<tr("GGC = 0, when all the nodes have exactly the same graph index.\n");
	ts<<tr("GGC = 1, when one node falls on all other geodesics between all the remaining (N-1) nodes. This is exactly the situation realised by a star graph.\n");
	ts<<"(Wasserman & Faust, formula 5.13, p. 192)\n\n";

	f.close();

	TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
	tempFileNameNoPath=fn.split( "/" );
	ed->setWindowTitle("Graph Centralities saved as: " + tempFileNameNoPath.last());
	ed->show();
}




/**
*	Writes Eccentricity Centralities into a file, then displays it.
*/
void MainWindow::slotCentralityEccentricity(){
	if (!fileLoaded && !networkModified  )  {
		QMessageBox::critical(this, "Error",tr("There are no nodes!\nLoad a network file or create a new network. \nThen ask me to compute something!"), "OK",0);

		statusBar()->showMessage( QString(tr(" Nothing to do...")) , statusBarDuration);
		return;
	}

	statusBar()->showMessage( QString(tr(" Please wait...")));
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

	activeGraph.createDistanceMatrix(true);	
	statusBar()->showMessage( QString(tr(" Finished with shortest-path distances...")));

	QString fn = "centrality_eccentricity.dat";
	QFile f( fn );
	if ( !f.open( QIODevice::WriteOnly ) )
		return;
	QTextStream ts(&f  );
	
	ts <<"-SocNetV- "<<VERSION<<"\n\n";
	ts <<tr("ECCENTRICITY- CENTRALITY REPORT \n");
	ts <<tr("Created: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
	ts<< tr("ECCENTRICITY CENTRALITY (EC) OF EACH NODE")<<"\n";
	ts<< tr("EC of a node u is the largest geodesic distance (u,t) for t in V")<<"\n"; 
	ts<< tr("Therefore, EC(u) reflects how far, at most, is each node from every other node.")<<"\n";
	ts<< tr("EC' is the standardized EC")<<"\n";
	ts<< tr("EC  range: 0 < EC < ")<<QString::number(activeGraph.maxIndexEC)<< tr(" (max geodesic distance)")<<"\n";
	ts<< tr("EC' range: 0 < EC'< 1 \n\n");
	ts << "Node"<<"\tEC\t\tEC'\t\t%EC\n";
	QList<Vertex*>::iterator it;
	for (it=activeGraph.m_graph.begin(); it!=activeGraph.m_graph.end(); it++){ 
		ts<<(*it)->name()<<"\t"<<(*it)->EC() << "\t\t"<< (*it)->SEC() << "\t\t" <<  (100* ((*it)->EC()) / activeGraph.sumEC)<<endl;
	}
	if (activeGraph.minEC == activeGraph.maxEC)
		ts << tr("\nAll nodes have the same EC value.\n");
	else {
		ts<<tr("\n Node ")<<activeGraph.maxNodeEC<< tr(" has the maximum EC value: ") <<activeGraph.maxEC <<"  \n";
		ts<<tr("\n Node ")<<activeGraph.minNodeEC<< tr(" has the minimum EC value: ") <<activeGraph.minEC <<"  \n";
	}

	ts << tr("\nThere are ")<<activeGraph.classesEC<<tr(" different eccentricity Centrality classes.\n");	
	ts<<tr("\nGROUP ECCENTRICITY CENTRALISATION (GEC)\n\n");
	ts<< tr("GEC = ") << activeGraph.groupEC<<"\n\n";
	ts<<tr("GEC range: 0 < GEC < 1\n");
	ts<<tr("GEC = 0, when all the nodes have exactly the same betweeness index.\n");
	ts<<tr("GEC = 1, when one node falls on all other geodesics between all the remaining (N-1) nodes. This is exactly the situation realised by a star graph.\n");
	ts<<"(Wasserman & Faust, formula 5.13, p. 192)\n\n";
	f.close();
	TextEditor *ed = new TextEditor(fn);        //OPEN A TEXT EDITOR WINDOW
	tempFileNameNoPath=fn.split( "/");
	ed->setWindowTitle("Eccentricity Centralities saved as: " + tempFileNameNoPath.last());
	ed->show();
	QApplication::restoreOverrideCursor();

}


/**
*	Called from Graph::
*/ 
bool MainWindow::showNumbers(){
	return displayNodeNumbersAct->isChecked();
}





/**
*  Turns on/off displaying the numbers of nodes (outside ones)
*/
void MainWindow::slotDisplayNodeNumbers(bool toggle) {
 	if (!fileLoaded && ! networkModified) {
		QMessageBox::critical(this, "Error",tr("There are no nodes! \nLoad a network file or create a new network."), "OK",0);
		statusBar()->showMessage(tr("Errr...no nodes here. Sorry!"), statusBarDuration);
		return;
	}
	statusBar()->showMessage(tr("Toggle Nodes Numbers. Please wait..."), statusBarDuration);

	if (!toggle) 	{
		graphicsWidget->setAllItemsVisibility(TypeNumber, false);
		statusBar()->showMessage(tr("Node Numbers are invisible now. Click the same option again to display them."), statusBarDuration);
		return;
	}
	else{
		graphicsWidget->setAllItemsVisibility(TypeNumber, true);
		statusBar()->showMessage(tr("Node Labels are visible again..."), statusBarDuration);
	}
}


/**
*	Called by Graph:: and this->initNet()
*/
bool MainWindow::showLabels(){
	return displayNodeLabelsAct->isChecked();
}



/**
*  Turns on/off displaying the labels of the nodes.
*/
void MainWindow::slotDisplayNodeLabels(bool toggle){
 	if (!fileLoaded && ! networkModified) {
		QMessageBox::critical(this, "Error",tr("There are no nodes! \nLoad a network file or create a new network first. "), "OK",0);
		statusBar()->showMessage(tr("No nodes found. Sorry..."), statusBarDuration);
		return;
	}
	statusBar()->showMessage(tr("Toggle Nodes Labels. Please wait..."), statusBarDuration);

	if (!toggle) 	{
		graphicsWidget->setAllItemsVisibility(TypeLabel, false);
		statusBar()->showMessage(tr("Node Labels are invisible now. Click the same option again to display them."), statusBarDuration);
		return;
	}
	else{
		graphicsWidget->setAllItemsVisibility(TypeLabel, true);
		statusBar()->showMessage(tr("Node Labels are visible again..."), statusBarDuration);
	}
	activeGraph.setShowLabels(toggle);
}




/**
*   Changes the size of all nodes
*/
void MainWindow::slotChangeAllNodesSize() {
	bool ok=FALSE;
	
	int newSize = QInputDialog::getInteger(this, "Change node size", tr("Select new size for all nodes: (1-16)"),initNodeSize, 1, 16, 1, &ok );
	if (!ok) {
		statusBar()->showMessage("Change node size operation cancelled.", statusBarDuration);
		return;
	}

	qDebug ("MW: slotChangeAllNodesSize:");
	changeAllNodesSize(newSize);
	graphChanged();
	statusBar()->showMessage (QString(tr("Ready")), statusBarDuration) ;
	return;
}


/**
*   Changes the size of nodes. 
*/
void MainWindow::changeAllNodesSize(int size) {
	qDebug ("MW: changeAllNodesSize:");
	if (size == 0 ) {
		if (activeNodes() < 200) {
			return;
		}
		else if (activeNodes() >= 200 && activeNodes() < 500){
			size = 4; 
		}
		else if (activeNodes() >= 500 && activeNodes() < 1000) {
			size = 3; 
		}
		else if (activeNodes() >= 1000) {
			size = 2; 
		}
	}
	initNodeSize = size;
	//graphicsWidget->setInitNodeSize(initNodeSize);
	activeGraph.setInitVertexSize(initNodeSize);
	qDebug ("MW: changeAllNodesSize: changing to %i", size);
	QList<QGraphicsItem *> list=scene->items();
	for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++) 
		if ( (*it) -> type() == TypeNode ){
		Node *jim = (Node*) (*it);
		(*jim).setSize(size);
	}
}


/**
*  Changes the shape of all nodes. 
*/
void MainWindow::slotChangeAllNodesShape() {
	bool ok=false;
	QStringList lst;
    	lst << "box"<< "circle"<< "diamond"<< "ellipse"<< "triangle";
    	QString newShape = QInputDialog::getItem(this, "Node shapes", "Select a shape for all nodes: ", lst, 1, TRUE, &ok);
	if ( ok ) {
	        //user selected an item and pressed OK
		QList<QGraphicsItem *> list=scene->items();
		for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
			if ( (*it) -> type() == TypeNode ){
				Node *jim = (Node*) (*it);
				(*jim).setShape(newShape);
				activeGraph.setVertexShape ((*jim).nodeNumber(), newShape);
			}
		graphChanged();
		activeGraph.setInitVertexShape(newShape);
		statusBar()->showMessage (QString(tr("All shapes have been changed. Ready")), statusBarDuration) ;
	} else {
		//user pressed Cancel
		statusBar()->showMessage (QString(tr("Change node shapes aborted...")), statusBarDuration) ;
	}

}



/**
*  Change size of all nodes' numbers (outside ones)
*/
void MainWindow::slotChangeNumbersSize() {
	bool ok=false;
	int newSize;
	newSize = QInputDialog::getInteger(this, "Change text size", tr("Change all nodenumbers size to: (1-16)"),initNumberSize, 1, 16, 1, &ok );
	if (!ok) {
		statusBar()->showMessage(tr("Change font size: Aborted."), statusBarDuration);
	return;
	}
	QList<QGraphicsItem *> list=scene->items();
	for (QList<QGraphicsItem *>::iterator it2=list.begin();it2!=list.end(); it2++)
		
		if ( (*it2)->type()==TypeNumber) {
		NodeNumber * number= (NodeNumber*) (*it2);
		qDebug ("MW: slotChangeNumbersSize Found");
		number->setFont( QFont (number->font().family(), newSize, QFont::Light, FALSE) );
	}
	statusBar()->showMessage(tr("Changed numbers size. Ready."), statusBarDuration);
}


/**
*  Changes size of all nodes' labels
*/
void MainWindow::slotChangeLabelsSize() {
/*  bool ok=false;
  int newSize;
  newSize= QInputDialog::getInteger("Change text size",
     tr("Choose a new font size for the labels: "),
     7, 7, 14 , 1, &ok, this )   ;
  if (!ok) {
     statusBar()->showMessage(tr("Change font size: Aborted."), statusBarDuration);
    return;
  }
  QList<QGraphicsItem *> list=scene->items();
  for (QList<QGraphicsItem *>::iterator it2=list.begin();it2!=list.end(); it2++)
     if ( (*it2)->type()==TypeLabel) {
       NodeLabel * number= (NodeLabel*) (*it2);
      number->setFont( QFont ("Helvetica", newSize, QFont::Normal, FALSE) );
     }

  statusBar()->showMessage(tr("Changed labels size. Ready."), statusBarDuration);*/
}





/**
	Turns on/off drawing links as thick as their weights.
	TODO 
*/
void MainWindow::slotDrawLinksThickAsWeights() {

}



/**
*  Turns on/off displaying link weight numbers
*/
void MainWindow::slotDisplayLinksWeightNumbers(bool toggle) {
 	if (!fileLoaded && ! networkModified) {
		QMessageBox::critical(this, "Error",tr("There are no links! \nLoad a network file or create a new network first."), "OK",0);
		statusBar()->showMessage(tr("No nodes or edges found. Sorry..."), statusBarDuration);
		return;
	}
	statusBar()->showMessage(tr("Toggle Edges Weights. Please wait..."), statusBarDuration);

	if (!toggle) 	{
		graphicsWidget->setAllItemsVisibility(TypeEdgeWeight, false);
		statusBar()->showMessage(tr("Edge weights are invisible now. Click the same option again to display them."), statusBarDuration);
		return;
	}
	else{
		graphicsWidget->setAllItemsVisibility(TypeEdgeWeight, true);
		statusBar()->showMessage(tr("Edge weights are visible again..."), statusBarDuration);
	}
	activeGraph.setShowLabels(toggle);


// 	pair<int,int> pair1;	
// 	QList<QGraphicsItem *> list=scene->items();
// 	if ( toggle )   {  //draw Edge Weight Numbers
// 		qDebug ("toogle is TRUE. Will show weight numbers");
// 		for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
// 			if ( (*it)->type() ==   Link_Rtti ){
// 				Edge* link= (Edge*) (*it);
// 				qDebug ("found link");
//  				EdgeWeight *linkWeight =new  EdgeWeight (link, QString::number(link->weight()), scene );
// 				qDebug ("will draw weight number %i",link->weight() );
// 				
//                			linkWeight ->setZ (255);
// 				pair1=link->center();
//                 		linkWeight ->move(pair1.first,pair1.second);
//                 		linkWeight-> setColor (link->color());
//                 		linkWeight ->show();
// 				qDebug ("show");
// 			}
// 	}
// 	else { //delete them
// 		qDebug ("toogle is FALSE. Deleting all numbers");
// 		for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++){
// 			if ( (*item)->type() ==   Weight_Rtti ) {
// 				delete *item;
// 			}
// 			else if ( (*item)->type() ==   Link_Rtti ) {
// 				Edge* link= (Edge*) (*item);
// 				link->clearWeightList();
// 				qDebug ("weight List cleared");
// 			}	
// 			
// 		}
// 		return;
// 	}

}



/**
*  Turns on/off displaying links
*/
void MainWindow::slotDisplayLinks(bool toggle){
	if (!fileLoaded && ! networkModified) {
		QMessageBox::critical(this, "Error",tr("There are no nodes nor links! \nLoad a network file or create a new network first!"), "OK",0);

		statusBar()->showMessage(tr("No links found..."), statusBarDuration);
		return;
	}
	statusBar()->showMessage(tr("Toggle Edges Arrows. Please wait..."), statusBarDuration);

	if (!toggle) 	{
		graphicsWidget->setAllItemsVisibility(TypeEdge, false);
		statusBar()->showMessage(tr("Links are invisible now. Click again the same menu to display them."), statusBarDuration);
		return;
	}
	else{
		graphicsWidget->setAllItemsVisibility(TypeEdge, true);
		statusBar()->showMessage(tr("Links visible again..."), statusBarDuration);
	}
	
}



/**
*  Turns on/off the arrows of links
*/
void MainWindow::slotDisplayLinksArrows(bool toggle){
	if (!fileLoaded && ! networkModified) {
		QMessageBox::critical(this, "Error",tr("There are no links! \nLoad a network file or create a new network first!"), "OK",0);

		statusBar()->showMessage(tr("No links found..."), statusBarDuration);
		return;
	}
	statusBar()->showMessage(tr("Toggle Edges Arrows. Please wait..."), statusBarDuration);

	if (!toggle) 	{
		QList<QGraphicsItem *> list = scene->items();
		for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++) {
			if ( (*item)->type() ==TypeEdge){
				Edge *edge = (Edge*) (*item);
				edge->showArrows(false);
				(*item)->hide();(*item)->show();
			}
		}
		return;
	}
	else{
		QList<QGraphicsItem *> list = scene->items();
		for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++)
			if ( (*item)->type() ==TypeEdge){
				Edge *edge = (Edge*) (*item);
				edge->showArrows(true);
				(*item)->hide();(*item)->show();
			}
	}
	statusBar()->showMessage(tr("Ready."));
}



/**
*  FIXME links Bezier 
*/
void MainWindow::slotDrawLinksBezier(bool toggle){
	if (!fileLoaded && ! networkModified) {
		QMessageBox::critical(this, "Error",tr("There are no links! \nLoad a network file or create a new network!"), "OK",0);

		statusBar()->showMessage(tr("There are NO links here!"), statusBarDuration);
		return;
	}
	statusBar()->showMessage(tr("Toggle links bezier. Please wait..."), statusBarDuration);
// //	graphicsWidget->setBezier(toggle);
	if (!toggle) 	{
// 		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
// 		QList<QGraphicsItem *> list = scene->items();
// 		for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++) {
// 			if ( (*item)->type() ==TypeEdge ){
// 				Edge *edge = (Edge*) (*item);
// //				edge->toggleBezier(false);
// 				(*item)->hide();(*item)->show();
// 			}
// 
// 		}
// 		QApplication::restoreOverrideCursor();
// 		return;
	}
	else{
// 		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
// 		QList<QGraphicsItem *> list = scene->items();
// 		for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++){
// 			if ( (*item)->type() ==TypeEdge ){
// 				Edge *edge = (Edge*) (*item);
// //				edge->toggleBezier(true);
// 				(*item)->hide();(*item)->show();
// 			}
// 		}
// 		QApplication::restoreOverrideCursor();
	}

}



/**
*  Changes the background color of the scene
*/
void MainWindow::slotBackgroundColor () {
	qDebug("MW: slotBackgroundColor ");
	QColor backgrColor = QColorDialog::getColor( initBackgroundColor, this );
	graphicsWidget ->setBackgroundBrush(QBrush(backgrColor));	
	statusBar()->showMessage(tr("Ready. "), statusBarDuration);
}


/**
*  Changes the color of all nodes
*/
void MainWindow::slotAllNodesColor(){
	bool ok=FALSE;
	initNodeColor = QInputDialog::getItem(this, "Nodes' colors", "Select a new color:", colorList, 1, TRUE, &ok);
    	if ( ok ) {
		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
		qDebug ("MW: Will change color");
		QList<QGraphicsItem *> list= scene->items();
		for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
			if ( (*it)->type() == TypeNode ) 	{
				Node *jim = (Node *) (*it);
				jim->setColor(initNodeColor);
				qDebug ("MW: Changed color");
				activeGraph.setVertexColor (jim->nodeNumber(), initNodeColor);
				graphChanged();
			}
//		graphicsWidget->setInitNodeColor(initNodeColor);
		activeGraph.setInitVertexColor(initNodeColor);
		QApplication::restoreOverrideCursor();
		statusBar()->showMessage(tr("Ready. ") ,statusBarDuration);
    	} 
	else {
	        // user pressed Cancel
		statusBar()->showMessage(tr("Change node color aborted. ") ,statusBarDuration);
    	}

}



/**
*  Changes the color of all links
*/
void MainWindow::slotAllLinksColor(){
	bool ok=FALSE;
	initLinkColor = QInputDialog::getItem(this, "Links' colors", "Select a new color:", colorList, 1, TRUE, &ok);
    	if ( ok ) {
		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
		qDebug ("MW: Will change color");
		QList<QGraphicsItem *> list= scene->items();
		for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
			if ( (*it)->type() == TypeEdge ) 	{
				Edge *link = (Edge *) (*it);
				link->setColor(initLinkColor );
				qDebug ("MW: Changed color");
				activeGraph.setEdgeColor (link->sourceNodeNumber(), link->targetNodeNumber(), initLinkColor);
				graphChanged();
			}
		activeGraph.setInitEdgeColor(initLinkColor);
		//graphicsWidget->setInitLinkColor(initLinkColor);
		QApplication::restoreOverrideCursor();
		statusBar()->showMessage(tr("Ready. ") ,statusBarDuration);

    	} 
	else {
	        // user pressed Cancel
		statusBar()->showMessage(tr("Change link color aborted. ") ,statusBarDuration);
    	}
}




/**
*  Changes the color of nodes' numbers
*/
void MainWindow::slotAllNumbersColor(){
	// = QInputDialog::getItem(this, "Links' colors", "Select a new color:", colorList, 1, TRUE, &ok);
	QColor textColor = QColorDialog::getColor( Qt::black, this );
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	qDebug ("MW: Will change color");
	QList<QGraphicsItem *> list= scene->items();
	for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
		if ( (*it)->type() == TypeNumber) 		{
			NodeNumber *jimNumber = (NodeNumber *) (*it);
			jimNumber->update();
			jimNumber->setDefaultTextColor(textColor);
		}
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(tr("Ready. ") ,statusBarDuration);
}



/**
*  Changes the color of nodes' labels
*/
void MainWindow::slotAllLabelsColor(){
	QColor textColor = QColorDialog::getColor( Qt::black, this );
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	qDebug ("MW: Will change label color");
	QList<QGraphicsItem *> list= scene->items();
	for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++)
		if ( (*it)->type() == TypeLabel) 		{
			NodeLabel *jimLabel = (NodeLabel *) (*it);
			jimLabel->update();
			jimLabel->setDefaultTextColor(textColor);
		}
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(tr("Label colors changed. Ready. ") ,statusBarDuration);
}



/**
*  turns antialiasing on or off
*/
void MainWindow::slotAntialiasing(bool toggle) {
	statusBar()->showMessage(tr("Toggle anti-aliasing. This will take some time if the network is large (>500)..."), statusBarDuration);
	//Inform graphicsWidget about the change
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
 	graphicsWidget->setRenderHint(QPainter::Antialiasing, toggle);
	graphicsWidget->setRenderHint(QPainter::TextAntialiasing, toggle);
	graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, toggle);
	QApplication::restoreOverrideCursor();
	if (!toggle) 
		statusBar()->showMessage(tr("Anti-aliasing off."), statusBarDuration);
	else  
		statusBar()->showMessage(tr("Anti-aliasing on."), statusBarDuration);
	
}

/**
*  turn progressbar on or off
*/
void MainWindow::slotShowProgressBar(bool toggle) {
	statusBar()->showMessage(tr("Toggle progressbar..."));
	if (!toggle)  {
		showProgressBar=FALSE;
		statusBar()->showMessage(tr("Progress bars off."), statusBarDuration);	
	}
	else   {
		showProgressBar=TRUE;
		statusBar()->showMessage(tr("Progress bars on."), statusBarDuration);	
	}
}



/**
*  Turns debugging messages on or off
*/
void MainWindow::slotPrintDebug(bool toggle){
	if (!toggle)   {
		printDebug=FALSE;
		statusBar()->showMessage(tr("Debug messages off."), statusBarDuration);
	}
	else  {
		printDebug=TRUE;
		statusBar()->showMessage(tr("Debug messages on."), statusBarDuration);
	}
}




/**
*  Turns Toolbar on or off
*/
void MainWindow::slotViewToolBar(bool toggle) {
	statusBar()->showMessage(tr("Toggle toolbar..."));
	if (toggle== false)   {
		toolBar->hide();
		statusBar()->showMessage(tr("Toolbar off."), statusBarDuration);	
	}
	else  {
		toolBar->show();
		statusBar()->showMessage(tr("Toolbar on."), statusBarDuration);	
	}
}



/**
*  Turns Statusbar on or off
*/
void MainWindow::slotViewStatusBar(bool toggle) {
	statusBar()->showMessage(tr("Toggle statusbar..."));

	if (toggle == false)   {
		statusBar()->hide();
		statusBar()->showMessage(tr("Status bar off."), statusBarDuration);	
	}
	else   {
		statusBar()->show();	
		statusBar()->showMessage(tr("Status bar on."), statusBarDuration);	
	}

}



/**
*  Displays a random tip
*/
void MainWindow::slotTips() {
	int randomTip=rand()%tipsCounter; //Pick a tip.
	QMessageBox::about( this, "Tip Of The Day", tips[randomTip]);
}



/**
	Creates our tips.
*/
void MainWindow::createTips(){
	tips+="You can add a new node by double-clicking on the scene.";	
	tips+="You can add a new node by clicking on Add button.";
	tips+="You can remove a node by clicking on Remove button.";		
	tips+="You can rotate the network by selecting a new angle on the dock.";	
	tips+="You can add a new link between two nodes, by middle-clicking (or pressing both mouse buttons simultanesously) on the first and then on the second node.";	
	tips+="You can remove a node by right-clicking on it and selecting Remove.";
	tips+="You can change background color (from the menu Edit > Colors).";
	tips+="Nodes can have the colors of your choice. Just right-click on a node and then select > Options > Change Color. You can select every color supported by the X.org pallette.";
	tips+="The dock on the left shows information about the network (nodes, edges, nodes with inlinks and nodes with outLinks) as well as information about any node you clicked on.";
	tips+="You can move a node easily by dragging it with your mouse.";
	tips+="SocNetV can save the positions of the nodes in a network, only if you save it in Pajek format.";
	tips+="You can apply layout algorithms on the network from the menu Layout or by clicking on the dock checkboxes";
	tips+="You can change the label of node by right-clicking on it, and selecting Options > Change Label.";
	tips+="The basic operations of SocNetV are available from the dock menu on the left, or by right-clicking on a node or a link.";
	tips+="Node information is displayed on the Status bar, when you left-click on it.";
	tips+="Link information is displayed on the Status bar, when you left-click on it.";

   	tipsCounter = 16;
}




/**
	Loads the HTML Help file and displays it via HTMLViewer.
*/
void MainWindow::slotHelp(){

	QString helpPath; 
	bool manualFound = FALSE;
	QDir d( QCoreApplication::applicationDirPath() );
	qDebug()<< QCoreApplication::applicationDirPath().toAscii();

	if ( d.exists("manual.html") ) { 
  		helpPath=d.filePath("manual.html");
	}
	else {
		if (d.dirName()=="bin") {
			d.cdUp();
		}
		if (d.cd("./manual") ) {
			if ( d.exists("manual.html") ) {
				helpPath=d.filePath("manual.html");
				manualFound = TRUE;	
			}
			else 	{
			      qDebug()<< "help file does not exist here.";
			      manualFound = FALSE;
		        }
		}
		if ( !manualFound && d.cd("/usr/local/share/doc/socnetv/") ) {			//for compile installation
			if (d.exists("manual/")) d.cd("manual/");
			if ( d.exists("manual.html") ) {
				helpPath=d.filePath("manual.html");
				qDebug()<< "path" << helpPath.toAscii();
				manualFound = TRUE;	
			}
			else {
				qDebug()<< "help file does not exist.";
				manualFound = FALSE;
			}
		}
               if (!manualFound && d.cd("/usr/share/doc/socnetv/") ) {     //for Debian Ubuntu
		       if (d.exists("manual/")) d.cd("manual/");
                       if ( d.exists("manual.html") ) {
                                helpPath=d.filePath("manual.html");
				manualFound = TRUE;	
                        }
                        else {
                                qDebug("help file does not exist in /usr/share/doc/socnetv/.");
				manualFound = FALSE;
                        }
                }             
               if ( !manualFound && d.cd("/usr/share/doc/packages/socnetv/") ) {  //for opensuse file hierarchy
		       if (d.exists("manual/")) d.cd("manual/");
                       if ( d.exists("manual.html") ) {
                                helpPath=d.filePath("manual.html");
				manualFound = TRUE;	
                        }
                        else {
                                qDebug("help file does not exist.");
                        }
                }
	      QString fedoraPath = "/usr/share/doc/socnetv-" + VERSION;
               if ( !manualFound && d.cd(fedoraPath) ) {  //for Fedora file hierarchy
		       if (d.exists("manual/")) d.cd("manual/");
                       if ( d.exists("manual.html") ) {
                                helpPath=d.filePath("manual.html");
				manualFound = TRUE;	
                        }
                        else {
                                qDebug("help file does not exist.");
                        }
                }



	}
        qDebug () << "help path is: " << helpPath.toAscii();
	HTMLViewer *helpViewer = new HTMLViewer (helpPath, this);
	helpViewer -> setWindowTitle ( "SocNetV "+ VERSION + tr(" Manual"));
	helpViewer->show();

}



/**
	Displays the following message!!
*/
void MainWindow::slotHelpAbout(){
     int randomCookie=rand()%fortuneCookiesCounter;//createFortuneCookies();
     QMessageBox::about( this, "About SocNetV",
	"<b>Soc</b>ial <b>Net</b>work <b>V</b>isualiser (SocNetV)"
	"<p><b>Version</b>: " + VERSION + "</p>"
	"<p><b>Build: </b> Sun, Feb 15, 2009</p>"
	
	"<p>(C) 2005-2009 by Dimitris V. Kalamaras"
	"<br> dimitris.kalamaras@gmail.com"

	"<p><b>Fortune cookie: </b><br> \""  + fortuneCookie[randomCookie]  +"\""
	
	
	"<p><b>License:</b><br>"

	"This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.</p>"

	"<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.</p>"

	"<p>You should have received a copy of the GNU General Public License along with this program; If not, see http://www.gnu.org/licenses/</p>");
}



/**
	Creates the fortune cookies displayed on the above message.
*/
void MainWindow::createFortuneCookies(){
	fortuneCookie+="sic itur ad astra / sic transit gloria mundi ? <br> --Unknown";
	fortuneCookie+="losers of yesterday, the winners of tomorrow... <br> --B.Brecht";
	fortuneCookie+="Patriotism is the virtue of the wicked... <br> --O. Wilde";
 
	fortuneCookie+="No tengo nunca mas, no tengo siempre. En la arena <br>" 
			"la victoria dejo sus piers perdidos.<br>"
		   	"Soy un pobre hombre dispuesto a amar a sus semejantes.<br>"
		   	"No se quien eres. Te amo. No doy, no vendo espinas. <br> --Pablo Neruda"  ;
	fortuneCookie+="I will never apologize for the United States of America. I don't care what it has done. I don't care what the facts are. <br> --Vice President George H.W. Bush, after the Iranian airliner flight IR655 (an Airbus A300) was shot down by a U.S. missile cruiser (USS Vincennes), killing all 290 civilian passengers...";
	fortuneCookie+="Man must not check reason by tradition, but contrawise, must check tradition by reason.<br> --Leo Tolstoy";
	fortuneCookie+="Only after the last tree has been cut down, <br>only after the last river has been poisoned,<br> only after the last fish has been caught,<br>only then will you realize that money cannot be eaten. <br> --The Cree People";
	fortuneCookie+="Stat rosa pristina nomine, nomina nuda tenemus <br > --Unknown";
	fortuneCookie+="Jupiter and Saturn, Oberon, Miranda"
					"And Titania, Neptune, Titan."
					"Stars can frighten. <br > Syd Barrett";

	fortuneCookiesCounter=9;
//   return fortuneCookie.count();
}




/**
	Displays a short message about the Qt Toolbox.
*/
void MainWindow::slotAboutQt(){
	QMessageBox::aboutQt(this, "About Qt - SocNetV");
}



