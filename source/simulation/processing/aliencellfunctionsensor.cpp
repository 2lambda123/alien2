#include "aliencellfunctionsensor.h"
#include "../entities/aliencell.h"
#include "../entities/aliencellcluster.h"

#include "../physics/physics.h"
#include "../../globaldata/simulationsettings.h"

#include <QtCore/qmath.h>
/*
#include <chrono>
#include <iostream>
using namespace std;
using namespace std::chrono;
*/
AlienCellFunctionSensor::AlienCellFunctionSensor()
{
}

AlienCellFunctionSensor::AlienCellFunctionSensor (quint8* cellTypeData)
{

}

AlienCellFunctionSensor::AlienCellFunctionSensor (QDataStream& stream)
{

}

void AlienCellFunctionSensor::execute (AlienToken* token, AlienCell* previousCell, AlienCell* cell, AlienGrid* grid, AlienEnergy*& newParticle, bool& decompose)
{
    AlienCellCluster* cluster(cell->getCluster());
    quint8 cmd = token->memory[static_cast<int>(SENSOR::IN)]%5;

    if( cmd == static_cast<int>(SENSOR_IN::DO_NOTHING) ) {
        token->memory[static_cast<int>(SENSOR::OUT)] = static_cast<int>(SENSOR_OUT::NOTHING_FOUND);
        return;
    }
    quint8 minMass = token->memory[static_cast<int>(SENSOR::IN_MIN_MASS)];
    quint8 maxMass = token->memory[static_cast<int>(SENSOR::IN_MAX_MASS)];
    qreal minMassReal = static_cast<qreal>(minMass);
    qreal maxMassReal = static_cast<qreal>(maxMass);
    if( maxMass == 0 )
        maxMassReal = 16000;    //large value => no max mass check

    //scanning vicinity?
    if( cmd == static_cast<int>(SENSOR_IN::SEARCH_VICINITY) ) {
        QVector3D cellPos = cell->calcPosition(grid);
//        auto time1 = high_resolution_clock::now();
        AlienCellCluster* otherCluster = grid->getNearbyClusterFast(cellPos,
                                                                    simulationParameters.CELL_FUNCTION_SENSOR_RANGE,
                                                                    minMassReal,
                                                                    maxMassReal,
                                                                    cell->getCluster());
//        nanoseconds diff1 = high_resolution_clock::now()- time1;
//        cout << "Dauer: " << diff1.count() << endl;
        if( otherCluster ) {
            token->memory[static_cast<int>(SENSOR::OUT)] = static_cast<int>(SENSOR_OUT::CLUSTER_FOUND);
            token->memory[static_cast<int>(SENSOR::OUT_MASS)] = convertURealToData(otherCluster->getMass());

            //calc relative angle
            QVector3D dir  = grid->displacement(otherCluster->getPosition(), cell->calcPosition()).normalized();
            qreal cellOrientationAngle = Physics::calcAngle(-cell->getRelPos() + previousCell->getRelPos());
            qreal relAngle = Physics::calcAngle(dir) - cellOrientationAngle - cluster->getAngle();
            token->memory[static_cast<int>(SENSOR::INOUT_ANGLE)] = convertAngleToData(relAngle);

            //calc distance by scanning along beam
            QVector3D beamPos = cell->calcPosition(grid);
            QVector3D scanPos;
            for(int d = 1; d < simulationParameters.CELL_FUNCTION_SENSOR_RANGE; d += 2) {
                beamPos += 2.0*dir;
                for(int rx = -1; rx < 2; ++rx)
                    for(int ry = -1; ry < 2; ++ry) {
                        scanPos = beamPos;
                        scanPos.setX(scanPos.x()+rx);
                        scanPos.setY(scanPos.y()+ry);
                        AlienCell* scanCell = grid->getCell(scanPos);
                        if( scanCell ) {
                            if( scanCell->getCluster() == otherCluster ) {
                                qreal dist = grid->displacement(cell->calcPosition(), scanCell->calcPosition()).length();
                                token->memory[static_cast<int>(SENSOR::OUT_DIST)] = convertURealToData(dist);
                                return;
                            }
                        }
                    }
            }
            token->memory[static_cast<int>(SENSOR::OUT)] = static_cast<int>(SENSOR_OUT::NOTHING_FOUND);
        }
        else
            token->memory[static_cast<int>(SENSOR::OUT)] = static_cast<int>(SENSOR_OUT::NOTHING_FOUND);
        return;
    }

    //scanning in a particular direction?
    QVector3D cellRelPos(cluster->calcPosition(cell)-cluster->getPosition());
    QVector3D dir(0.0, 0.0, 0.0);
    if( cmd == static_cast<int>(SENSOR_IN::SEARCH_BY_ANGLE) ) {
        qreal relAngle = convertDataToAngle(token->memory[static_cast<int>(SENSOR::INOUT_ANGLE)]);
        qreal angle = Physics::calcAngle(-cell->getRelPos() + previousCell->getRelPos()) + cluster->getAngle() + relAngle;
        dir = Physics::calcVector(angle);
    }
    if( cmd == static_cast<int>(SENSOR_IN::SEARCH_FROM_CENTER) ) {
        dir = cellRelPos.normalized();
    }
    if( cmd == static_cast<int>(SENSOR_IN::SEARCH_TOWARD_CENTER) ) {
        dir = -cellRelPos.normalized();
    }

    //scan along beam
    QList< AlienCell* > hitListCell;
    QVector3D beamPos = cell->calcPosition(grid);
    QVector3D scanPos;
    for(int d = 1; d < simulationParameters.CELL_FUNCTION_SENSOR_RANGE; d += 2) {
        beamPos += 2.0*dir;
        for(int rx = -1; rx < 2; ++rx)
            for(int ry = -1; ry < 2; ++ry) {
                scanPos = beamPos;
                scanPos.setX(scanPos.x()+rx);
                scanPos.setY(scanPos.y()+ry);
                AlienCell* scanCell = grid->getCell(scanPos);
                if( scanCell ) {
                    if( scanCell->getCluster() != cell->getCluster() ) {

                        //scan masses
                        qreal mass = scanCell->getCluster()->getMass();
                        if( mass >= (minMassReal-ALIEN_PRECISION) && mass <= (maxMassReal+ALIEN_PRECISION) )
                            hitListCell << scanCell;
                    }
                }
            }

        //strike?
        if( !hitListCell.isEmpty() ) {

            //find largest cluster
            qreal largestMass = 0.0;
            AlienCell* largestClusterCell = 0;
            foreach( AlienCell* hitCell, hitListCell) {
                if( hitCell->getCluster()->getMass() > largestMass ) {
                    largestMass = hitCell->getCluster()->getMass();
                    largestClusterCell = hitCell;
                }
            }
            token->memory[static_cast<int>(SENSOR::OUT)] = static_cast<int>(SENSOR_OUT::CLUSTER_FOUND);
            qreal dist = grid->displacement(cell->calcPosition(), largestClusterCell->calcPosition()).length();
            token->memory[static_cast<int>(SENSOR::OUT_DIST)] = convertURealToData(dist);
            token->memory[static_cast<int>(SENSOR::OUT_MASS)] = convertURealToData(largestClusterCell->getCluster()->getMass());
//            token->memory[static_cast<int>(SENSOR::INOUT_ANGLE)] = convertURealToData(relAngle);
            return;
        }
    }
    token->memory[static_cast<int>(SENSOR::OUT)] = static_cast<int>(SENSOR_OUT::NOTHING_FOUND);
}

QString AlienCellFunctionSensor::getCellFunctionName () const
{
    return "SENSOR";
}

void AlienCellFunctionSensor::serialize (QDataStream& stream)
{
    AlienCellFunction::serialize(stream);
}
