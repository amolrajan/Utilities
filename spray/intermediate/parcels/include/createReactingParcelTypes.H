/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2008-2010 OpenCFD Ltd.
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#ifndef createReactingParcelTypes_H
#define createReactingParcelTypes_H

#include "thermoPhysicsTypes.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#define createReactingParcelType(ParcelType)                                  \
                                                                              \
    createReactingParcelThermoType(ParcelType, constGasThermoPhysics);        \
    createReactingParcelThermoType(ParcelType, gasThermoPhysics);             \
    createReactingParcelThermoType(ParcelType, icoPoly8ThermoPhysics);


#define createReactingParcelThermoType(ParcelType, ThermoType)                \
                                                                              \
    typedef ParcelType<ThermoType> ParcelType##ThermoType;                    \
                                                                              \
    defineTemplateTypeNameAndDebug(ParcelType##ThermoType, 0);                \
    defineTemplateTypeNameAndDebug(Particle<ParcelType##ThermoType>, 0);      \
    defineTemplateTypeNameAndDebug(Cloud<ParcelType##ThermoType>, 0);         \
                                                                              \
    defineParcelTypeNameAndDebug(KinematicParcel<ParcelType##ThermoType>, 0); \
    defineTemplateTypeNameAndDebug                                            \
    (                                                                         \
        KinematicParcel<ParcelType##ThermoType>,                              \
        0                                                                     \
    );                                                                        \
    defineParcelTypeNameAndDebug(ThermoParcel<ParcelType##ThermoType>, 0);    \
    defineTemplateTypeNameAndDebug(ThermoParcel<ParcelType##ThermoType>, 0);  \
    defineParcelTypeNameAndDebug(ReactingParcel<ParcelType##ThermoType>, 0);  \
    defineTemplateTypeNameAndDebug(ReactingParcel<ParcelType##ThermoType>, 0);\
                                                                              \
    defineParcelTypeNameAndDebug(KinematicCloud<ParcelType##ThermoType>, 0);  \
    defineParcelTypeNameAndDebug(ThermoCloud<ParcelType##ThermoType>, 0);     \
    defineParcelTypeNameAndDebug(ReactingCloud<ParcelType##ThermoType>, 0);


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
