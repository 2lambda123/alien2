#ifndef CELLFUNCTIONCONSTRUCTOR_H
#define CELLFUNCTIONCONSTRUCTOR_H

#include "Model/Features/CellFunction.h"

#include <QVector2D>

class CellConstructorImpl
	: public CellFunction
{
public:
    CellConstructorImpl (UnitContext* context);

    Enums::CellFunction::Type getType () const { return Enums::CellFunction::CONSTRUCTOR; }

protected:
    ProcessingResult processImpl (Token* token, Cell* cell, Cell* previousCell) override;

private:
};

#endif // CELLFUNCTIONCONSTRUCTOR_H
