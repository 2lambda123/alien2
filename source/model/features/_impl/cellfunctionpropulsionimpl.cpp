#include "cellfunctionpropulsionimpl.h"

#include "model/entities/cell.h"
#include "model/entities/cellcluster.h"
#include "model/entities/energyparticle.h"
#include "model/entities/token.h"
#include "model/physics/physics.h"
#include "model/physics/codingphysicalquantities.h"
#include "model/config.h"

#include <QtCore/qmath.h>


CellFunctionPropulsionImpl::CellFunctionPropulsionImpl (SimulationContext* context)
    : CellFunction(context)
{
}

namespace {
    qreal convertDataToThrustPower (quint8 b)
    {
        return 1/10000.0*((qreal)b+10.0);
    }
}

CellFeature::ProcessingResult CellFunctionPropulsionImpl::processImpl (Token* token, Cell* cell, Cell* previousCell)
{
    ProcessingResult processingResult {false, 0};
    CellCluster* cluster(cell->getCluster());
    quint8 cmd = token->memory[static_cast<int>(PROP::IN)]%7;
    qreal angle = CodingPhysicalQuantities::convertDataToAngle(token->memory[static_cast<int>(PROP::IN_ANGLE)]);
    qreal power = convertDataToThrustPower(token->memory[static_cast<int>(PROP::IN_POWER)]);

    if( cmd == static_cast<int>(PROP_IN::DO_NOTHING) ) {
        token->memory[static_cast<int>(PROP::OUT)] = static_cast<int>(PROP_OUT::SUCCESS);
        return processingResult;
    }

    //calc old kinetic energy
    qreal eKinOld(Physics::kineticEnergy(cluster->getMass(), cluster->getVel(), cluster->getAngularMass(), cluster->getAngularVel()));

    //calc old tangential velocity
    QVector3D cellRelPos(cluster->calcPosition(cell)-cluster->getPosition());
    QVector3D tangVel(Physics::tangentialVelocity(cellRelPos, cluster->getVel(), cluster->getAngularVel()));

    //calc impulse angle
    QVector3D impulse(0.0, 0.0, 0.0);
    if( cmd == static_cast<int>(PROP_IN::BY_ANGLE) ) {
        qreal thrustAngle = (Physics::angleOfVector(-cell->getRelPos() + previousCell->getRelPos())+cluster->getAngle()+ angle)*degToRad;
        impulse = QVector3D(qSin(thrustAngle), -qCos(thrustAngle), 0.0)*power;
    }
    if( cmd == static_cast<int>(PROP_IN::FROM_CENTER) ) {
        impulse = cellRelPos.normalized()*power;
    }
    if( cmd == static_cast<int>(PROP_IN::TOWARD_CENTER) ) {
        impulse = -cellRelPos.normalized()*power;
    }

    QVector3D rAPp = cellRelPos;
//    grid->correctDistance(rAPp);
    rAPp = Physics::rotateQuarterCounterClockwise(rAPp);
    if( cmd == static_cast<int>(PROP_IN::ROTATION_CLOCKWISE) ) {
        impulse = -rAPp.normalized()*power;
    }
    if( cmd == static_cast<int>(PROP_IN::ROTATION_COUNTERCLOCKWISE) ) {
        impulse = rAPp.normalized()*power;
    }
    if( cmd == static_cast<int>(PROP_IN::DAMP_ROTATION) ) {
        if( cluster->getAngularVel() > 0.00 )
            impulse = rAPp.normalized()*power;
        if( cluster->getAngularVel() < 0.00 )
            impulse = -rAPp.normalized()*power;
    }

    //calc impact of impulse to cell structure
    QVector3D newVel;
    qreal newAngularVel;
    Physics::applyImpulse(impulse, rAPp, cluster->getMass(), cluster->getVel(), cluster->getAngularMass(), cluster->getAngularVel(), newVel, newAngularVel);

    //only for damping: prove if its too much
    if( cmd == static_cast<int>(PROP_IN::DAMP_ROTATION) ) {
        if( (cluster->getAngularVel() > 0.0 && newAngularVel < 0.0)
                || (cluster->getAngularVel() < 0.0 && newAngularVel > 0.0) ) {
            newVel = cluster->getVel();
            newAngularVel = cluster->getAngularVel();

            //update return value
            token->memory[static_cast<int>(PROP::OUT)] = static_cast<int>(PROP_OUT::SUCCESS_DAMPING_FINISHED);
            return processingResult;
        }
    }

    //calc new kinetic energy
    qreal eKinNew(Physics::kineticEnergy(cluster->getMass(), newVel, cluster->getAngularMass(), newAngularVel));
    qreal energyDiff((eKinNew-eKinOld)/simulationParameters.INTERNAL_TO_KINETIC_ENERGY);

    //has token enough energy?
    if( token->energy >= (energyDiff + qAbs(energyDiff) + simulationParameters.MIN_TOKEN_ENERGY + ALIEN_PRECISION) ) {

        //create energy particle with difference energy
        processingResult.newEnergyParticle = new EnergyParticle(qAbs(energyDiff), cluster->calcPosition(cell, _context)-impulse.normalized()
            , tangVel-impulse.normalized()/4.0, _context);

        //update velocities
        cluster->setVel(newVel);
        cluster->setAngularVel(newAngularVel);
        token->energy = token->energy - (energyDiff+qAbs(energyDiff));

        //update return value
        token->memory[static_cast<int>(PROP::OUT)] = static_cast<int>(PROP_OUT::SUCCESS);
    }
    else {

        //update return value
        token->memory[static_cast<int>(PROP::OUT)] = static_cast<int>(PROP_OUT::ERROR_NO_ENERGY);
    }
    return processingResult;
}

