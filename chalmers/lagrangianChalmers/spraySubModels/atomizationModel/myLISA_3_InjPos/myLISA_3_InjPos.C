/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 1991-2009 OpenCFD Ltd.
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

\*---------------------------------------------------------------------------*/

#include "error.H"

#include "myLISA_3_InjPos.H"
#include "addToRunTimeSelectionTable.H"
#include "basicMultiComponentMixture.H"

#include "RosinRammler.H"

//------------------------------------------CH 101108------------------------//
#include "injectorModel.H"
//---------------------------------------END--------------------------------//
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(myLISA_3_InjPos, 0);

addToRunTimeSelectionTable
(
    atomizationModel,
    myLISA_3_InjPos,
    dictionary
);

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

// Construct from components
myLISA_3_InjPos::myLISA_3_InjPos
(
    const dictionary& dict,
    spray& sm
)
:
    atomizationModel(dict, sm),
    coeffsDict_(dict.subDict(typeName+"Coeffs")),
    rndGen_(sm.rndGen()),
    Cl_(readScalar(coeffsDict_.lookup("Cl"))),
    cTau_(readScalar(coeffsDict_.lookup("cTau"))),
    Q_(readScalar(coeffsDict_.lookup("Q"))),
    J_(readScalar(coeffsDict_.lookup("J"))),
    lisaExp_(readScalar(coeffsDict_.lookup("lisaExp")))
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

myLISA_3_InjPos::~myLISA_3_InjPos()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void myLISA_3_InjPos::atomizeParcel
(
    parcel& p,
    const scalar deltaT,
    const vector& vel,
    const liquidMixture& fuels
) const
{


    const PtrList<volScalarField>& Y = spray_.composition().Y();

    label Ns = Y.size();
    label cellI = p.cell();
    scalar pressure = spray_.p()[cellI];
    scalar temperature = spray_.T()[cellI];
//--------------------------------------AL____101015--------------------------------//
//  scalar Taverage = p.T() + (temperature - p.T())/3.0;
    scalar Taverage = temperature;
//-----------------------------------------END--------------------------------------//
    scalar Winv = 0.0;

    for(label i=0; i<Ns; i++)
    {
        Winv += Y[i][cellI]/spray_.gasProperties()[i].W();
    }

    scalar R = specie::RR*Winv;

    // ideal gas law to evaluate density
    scalar rhoAverage = pressure/R/Taverage;
    //scalar nuAverage = muAverage/rhoAverage;
    scalar sigma = fuels.sigma(pressure, p.T(), p.X());

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
    //     The We and Re numbers are to be evaluated using the 1/3 rule.
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    scalar WeberNumber = p.We(vel, rhoAverage, sigma);

    scalar tau = 0.0;
    scalar dL = 0.0;
    scalar k = 0.0;
    scalar muFuel = fuels.mu(pressure, p.T(), p.X());
    scalar rhoFuel = fuels.rho(1.0e+5, p.T(), p.X());
    scalar nuFuel = muFuel/rhoFuel;

    vector uDir = p.U()/mag(p.U());

    scalar uGas = mag(vel & uDir);
    vector Ug = uGas*uDir;

/*
    TL
    It might be the relative velocity between Liquid and Gas, but I use the
    absolute velocity of the parcel as suggested by the authors
*/

//    scalar U = mag(p.Urel(vel));
    scalar U = mag(p.U());
    p.ct() += deltaT;

    scalar Q = rhoAverage/rhoFuel;

    const injectorType& it =
        spray_.injectors()[label(p.injector())].properties();

    if (it.nHoles() > 1)
    {
        Info << "Warning: This atomization model is not suitable for multihole injector." << endl
             << "Only the first hole will be used." << endl;
    }

    const vector direction = it.direction(0, spray_.runTime().value());
//--------------------------------CH 101108--------------------------------------------------//
//    const vector itPosition = it.position(0);
      const injectorModel& im = spray_.injection();
      const vector itPosition = it.position(0) + im.injDist(0)*direction/mag(direction);
//------------------------------------END----------------------------------------------------//
    scalar pWalk = mag(p.position() - itPosition);

//  Updating liquid sheet tickness... that is the droplet diameter

//    const vector direction = it.direction(0, spray_.runTime().value());

    scalar h = (p.position() - itPosition) & direction;

    scalar d = sqrt(sqr(pWalk)-sqr(h));

    scalar time = pWalk/mag(p.U());

    scalar elapsedTime = spray_.runTime().value();

    scalar massFlow = it.massFlowRate(max(0.0,elapsedTime-time));

    scalar hSheet = massFlow/(mathematicalConstant::pi*d*rhoFuel*mag(p.U()));

    p.d() = min(hSheet,p.d());

    if(WeberNumber > 27.0/16.0)
    {

        scalar kPos = 0.0;
        scalar kNeg = Q*pow(U, 2.0)*rhoFuel/sigma;

        scalar derivativePos = sqrt
        (
            Q*pow(U,2.0)
        );

        scalar derivativeNeg =
        (
            8.0*pow(nuFuel, 2.0)*pow(kNeg, 3.0)
            + Q*pow(U, 2.0)*kNeg
            - 3.0*sigma/2.0/rhoFuel*pow(kNeg, 2.0)
        )
        /
        sqrt
        (
            4.0*pow(nuFuel, 2.0)*pow(kNeg, 4.0)
            + Q*pow(U, 2.0)*pow(kNeg, 2.0)
            - sigma*pow(kNeg, 3.0)/rhoFuel
        )
        -
        4.0*nuFuel*kNeg;

        scalar kOld = 0.0;


        for(label i=0; i<40; i++)
        {

            k = kPos - (derivativePos/((derivativeNeg-derivativePos)/(kNeg-kPos)));

            scalar derivativek =
            (
                8.0*pow(nuFuel, 2.0)*pow(k, 3.0)
                + Q*pow(U, 2.0)*k
                - 3.0*sigma/2.0/rhoFuel*pow(k, 2.0)
            )
            /
            sqrt
            (
                4.0*pow(nuFuel, 2.0)*pow(k, 4.0)
                + Q*pow(U, 2.0)*pow(k, 2.0)
                - sigma*pow(k, 3.0)/rhoFuel
            )
            -
            4.0*nuFuel*k;

            if(derivativek > 0)
            {
                derivativePos = derivativek;
                kPos = k;
            }
            else
            {
                derivativeNeg = derivativek;
                kNeg = k;
            }

            if(mag(k-kOld)/k < 1e-4)
            {
                break;
            }

            kOld = k;

        }

        scalar omegaS =
        - 2.0 * nuFuel * pow(k, 2.0)
        + sqrt
        (
                4.0*pow(nuFuel, 2.0)*pow(k, 4.0)
            +   Q*pow(U, 2.0)*pow(k, 2.0)
            -   sigma*pow(k, 3.0)/rhoFuel
        );

        tau = cTau_/omegaS;
        dL = sqrt(8.0*p.d()/k);

    }
    else
    {

        k =
        rhoAverage*pow(U, 2.0)
        /
        2.0*sigma;

//--------------------------------------AL____101011--------------------------------//
//      scalar J = pWalk*p.d()/2.0;
        scalar J = time*hSheet/2.0;
//-----------------------------------------END--------------------------------------//

        tau = pow(3.0*cTau_,2.0/3.0)*cbrt(J*sigma/(sqr(Q)*pow(U,4.0)*rhoFuel));

        dL = sqrt(4.0*p.d()/k);
    }



    scalar kL =
        1.0
        /
        (
            dL *
            pow(0.5 + 1.5 * muFuel/pow((rhoFuel*sigma*dL), 0.5), 0.5)
        );

    scalar dD = cbrt(3.0*mathematicalConstant::pi*pow(dL, 2.0)/kL);
// lisaExp is included in coeffsDict
//    scalar lisaExp = 0.27;
    scalar ambientPressure = 1.0e+5;

    scalar pRatio = spray_.ambientPressure()/ambientPressure;

    dD = dD*pow(pRatio,lisaExp_);

//  modifications to take account of the flash boiling on primary breakUp

    scalar pExp = 0.135;

    scalar chi = 0.0;

    label Nf = fuels.components().size();

    scalar Td = p.T();

    for(label i = 0; i < Nf ; i++)
    {

        if(fuels.properties()[i].pv(spray_.ambientPressure(), Td) >= 0.999*spray_.ambientPressure())
        {

//          The fuel is boiling.....
//          Calculation of the boiling temperature

            scalar tBoilingSurface = Td;

            label Niter = 200;

            for(label k=0; k< Niter ; k++)
            {
                scalar pBoil = fuels.properties()[i].pv(pressure, tBoilingSurface);

                if(pBoil > pressure)
                {
                    tBoilingSurface = tBoilingSurface - (Td-temperature)/Niter;
                }
                else
                {
                    break;
                }
            }

            scalar hl = fuels.properties()[i].hl(spray_.ambientPressure(), tBoilingSurface);
            scalar iTp = fuels.properties()[i].h(spray_.ambientPressure(), Td) - spray_.ambientPressure()/fuels.properties()[i].rho(spray_.ambientPressure(), Td);
            scalar iTb = fuels.properties()[i].h(spray_.ambientPressure(), tBoilingSurface) - spray_.ambientPressure()/fuels.properties()[i].rho(spray_.ambientPressure(), tBoilingSurface);

            chi += p.X()[i]*(iTp-iTb)/hl;

        }
    }

    //  bounding chi

    chi = max(chi, 0.0);
    chi = min(chi, 1.0);

    //  modifing dD to take account of flash boiling

    dD = dD*(1.0 - chi*pow(pRatio, -pExp));
    scalar lBU = Cl_ * mag(p.U())*tau;
    if(pWalk > lBU)
    {

        p.liquidCore() = 0.0;

//      calculate the new diameter with the standard 1D Rosin Rammler distribution

//--------------------------------AL_____101012------------------------------//
// Calculation of the mean radius based on SMR rs. Coefficient factorGamma depends on nExp.
// Note that Reitz either used (Schmidt et al., 1999-01-0496) or skipped (Senecal et al.) this factor!!!
//      scalar factorGamma = 0.75*sqrt(mathematicalConstant::pi);       //nExp=2
	scalar factorGamma = 1.;
        scalar delta = dD/factorGamma;

/*      dD is the SMD, and the delta is calculated using gama
        function. Here we assume nExp = 2. */
//      scalar delta = dD/(0.75*sqrt(mathematicalConstant::pi));

//        scalar minValue = min(p.d()/20.0,dD/20.0);
        scalar minValue = dD/10.0;

//      delta is divided by 20 instead of 10 in order to make sure of small minValue
//      scalar minValue = min(p.d(),dD/20.0);

//        scalar maxValue = p.d();
        scalar maxValue = dD;

//      The pdf value for 4.0*delta is already very small.
//      scalar maxValue = delta*4.0;

        if(maxValue - minValue < SMALL)
        {
//          minValue = p.d()/20.0;
            minValue = maxValue/20.0;
//-----------------------------------END-------------------------------------//
        }

        scalar range = maxValue - minValue;

        scalar nExp = 3;
        scalar rrd_[500];
//--------------------------------AL_____101012------------------------------//
        scalar probFactorMin = exp(-pow(minValue/delta,nExp));
        scalar probFactorMax = exp(-pow(maxValue/delta,nExp));
        scalar probFactor = 1./(probFactorMin - probFactorMax);
//-----------------------------------END-------------------------------------//
        for(label n=0; n<500; n++)
        {
            scalar xx = minValue + range*n/500;
//-------------------------------AL_____101012-------------------------------//
//          rrd_[n] = 1 - exp(-pow(xx/delta,nExp));
            rrd_[n] = (probFactorMin - exp(-pow(xx/delta,nExp)))*probFactor;
//-----------------------------------END-------------------------------------//
        }


        bool success = false;
        scalar x = 0;
        scalar y = rndGen_.scalar01();
        label k = 0;

        while(!success && (k<500))
        {
            if (rrd_[k]>y)
            {
                success = true;
            }
            k++;

        }
//--------------------------------AL_____101012------------------------------//
//      x = minValue + range*n/500;
        x = minValue + range*(k-0.5)/500.0;
//------------------------------------END------------------------------------//
//  New droplet diameter

        p.d() = x;
        p.ct() = 0.0;

    }


}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
