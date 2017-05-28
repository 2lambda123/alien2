#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QByteArray>
#include <QStack>

#include "Definitions.h"
#include "Model/Definitions.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow (SimulationController* simulator, SimulationAccess* access, QWidget *parent = 0);
    ~MainWindow ();

private Q_SLOTS:

    //menu: simulation
    void newSimulation ();
    void loadSimulation ();
	void saveSimulation();
    void runClicked (bool run);
    void stepForwardClicked ();
    void stepBackClicked ();
    void snapshotUniverse ();
    void restoreUniverse ();

	//menu: settings
	void editSimulationParameters();
	void loadSimulationParameters();
	void saveSimulationParameters();
	void editSymbolTable();
	void loadSymbols();
	void saveSymbols();
	void loadSymbolsWithMerging();

    //menu: view
    void setVisualMode (bool shapeUniverse);
    void alienMonitorTriggered (bool on);
    void alienMonitorClosed();
    void fullscreen (bool triggered);

    //menu: particle
    void addCell ();
    void addEnergyParticle ();
    void addRandomEnergy ();
    void copyCell ();
    void pasteCell ();

    //menu: ensemble
    void addBlockStructure ();
    void addHexagonStructure ();
    void loadExtendedSelection ();
    void saveExtendedSelection ();
    void copyExtendedSelection ();
    void pasteExtendedSelection ();
    void multiplyRandomExtendedSelection ();
    void multiplyArrangementExtendedSelection ();

    //menu: help
    void aboutAlien ();
    void tutorialClosed();

    //misc
    void oneSecondTimeout ();
    void fpsForcingButtonClicked (bool toggled);
    void fpsForcingSpinboxClicked ();
    void numTokenChanged (int numToken, int maxToken, bool pasteTokenPossible);

    void cellFocused (Cell* cell);
    void cellDefocused ();
    void energyParticleFocused (EnergyParticle* e);
    void entitiesSelected(int numCells, int numEnergyParticles);

	void updateFrameLabel();
    void startScreenFinished ();

private:
    Ui::MainWindow *ui;

    void changeEvent(QEvent *e);
	void stopSimulation();
	void updateControllerAndEditors();

    SimulationController* _simController;
	TextEditor* _textEditor;

    QTimer* _oneSecondTimer;
    SimulationMonitor* _monitor;
    TutorialWindow* _tutorialWindow;
    StartScreenController* _startScreen;
	struct Framedata {
		int fps = 0;
		int frame = 0;
	} _framedata;

    QByteArray _serializedEnsembleData;
    QByteArray _serializedCellData;

    QStack< QByteArray > _undoUniverserses;

    QByteArray _snapshot;
};

#endif // MAINWINDOW_H
