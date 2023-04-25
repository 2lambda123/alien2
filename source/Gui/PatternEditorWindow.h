#pragma once

#include "EngineInterface/Definitions.h"
#include "EngineInterface/SelectionShallowData.h"
#include "EngineInterface/Descriptions.h"
#include "Definitions.h"
#include "AlienWindow.h"

class _PatternEditorWindow : public _AlienWindow
{
public:
    _PatternEditorWindow(
        EditorModel const& editorModel,
        SimulationController const& simController,
        Viewport const& viewport,
        EditorControllerWeakPtr const& editorController);

    bool isObjectInspectionPossible() const;
    bool isGenomeInspectionPossible() const;

    bool isCopyingPossible() const;
    void onCopy();
    bool isPastingPossible() const;
    void onPaste();
    bool isDeletingPossible() const;
    void onDelete();

private:
    void processIntern() override;

    void onGenerateExecutionOrderNumbers();
    void onMakeSticky();
    void onRemoveStickiness();
    void onSetBarrier(bool value);
    bool colorButton(std::string id, uint32_t cellColor);
    bool hasSelectionChanged(SelectionShallowData const& selection) const;

    EditorModel _editorModel;
    SimulationController _simController;
    Viewport _viewport;
    OpenPatternDialog _openPatternDialog;
    SavePatternDialog _savePatternDialog;
    EditorControllerWeakPtr _editorController;

    float _angle = 0;
    float _angularVel = 0;
    std::optional<SelectionShallowData> _lastSelection;
    std::optional<DataDescription> _copiedSelection;
};