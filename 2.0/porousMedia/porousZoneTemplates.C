/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2004-2010 OpenCFD Ltd.
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

\*----------------------------------------------------------------------------*/

#include "porousZone.H"
#include "fvMesh.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

template<class Type>
void Foam::porousZone::modifyDdt(fvMatrix<Type>& m) const
{
    if (porosity_ < 1)
    {
        forAll(cellZoneIds_, zoneI)
        {
            const labelList& cells = mesh_.cellZones()[cellZoneIds_[zoneI]];

            forAll(cells, i)
            {
                m.diag()[cells[i]]   *= porosity_;
                m.source()[cells[i]] *= porosity_;
            }
        }
    }
}


template<class RhoFieldType>
void Foam::porousZone::addPowerLawResistance
(
    scalarField& Udiag,
    const scalarField& V,
    const RhoFieldType& rho,
    const vectorField& U
) const
{
    const scalar C0 = C0_;
    const scalar C1m1b2 = (C1_ - 1.0)/2.0;

    forAll(cellZoneIds_, zoneI)
    {
        const labelList& cells = mesh_.cellZones()[cellZoneIds_[zoneI]];

        forAll(cells, i)
        {
            Udiag[cells[i]] +=
                V[cells[i]]*rho[cells[i]]*C0*pow(magSqr(U[cells[i]]), C1m1b2);
        }
    }
}


template<class RhoFieldType>
void Foam::porousZone::addViscousInertialResistance
(
    scalarField& Udiag,
    vectorField& Usource,
    const scalarField& V,
    const RhoFieldType& rho,
    const RhoFieldType& T,
    const scalarField& mu,
    const vectorField& U
) const
{
    const tensor& D = D_.value();
    const tensor& F = F_.value();

    forAll(cellZoneIds_, zoneI)
    {
        const labelList& cells = mesh_.cellZones()[cellZoneIds_[zoneI]];

        forAll(cells, i)
        {
	  //const tensor dragCoeff = mu[cells[i]]*D
	  //+ (rho[cells[i]]*mag(U[cells[i]]))*F;
	    const tensor dragCoeff = mu[cells[i]]*::pow(T[cells[i]], DTExp_)*D + (::pow(T[cells[i]], FTExp_)*rho[cells[i]]*mag(U[cells[i]]))*F;

            const scalar isoDragCoeff = tr(dragCoeff);

            Udiag[cells[i]] += V[cells[i]]*isoDragCoeff;
            Usource[cells[i]] -=
                V[cells[i]]*((dragCoeff - I*isoDragCoeff) & U[cells[i]]);
        }
    }
}


template<class RhoFieldType>
void Foam::porousZone::addPowerLawResistance
(
    tensorField& AU,
    const RhoFieldType& rho,
    const vectorField& U
) const
{
    const scalar C0 = C0_;
    const scalar C1m1b2 = (C1_ - 1.0)/2.0;

    forAll(cellZoneIds_, zoneI)
    {
        const labelList& cells = mesh_.cellZones()[cellZoneIds_[zoneI]];

        forAll(cells, i)
        {
            AU[cells[i]] = AU[cells[i]]
                + I*(rho[cells[i]]*C0*pow(magSqr(U[cells[i]]), C1m1b2));
        }
    }
}


template<class RhoFieldType>
void Foam::porousZone::addViscousInertialResistance
(
    tensorField& AU,
    const RhoFieldType& rho,
    const RhoFieldType& T,
    const scalarField& mu,
    const vectorField& U
) const
{
    const tensor& D = D_.value();
    const tensor& F = F_.value();

    forAll(cellZoneIds_, zoneI)
    {
        const labelList& cells = mesh_.cellZones()[cellZoneIds_[zoneI]];

        forAll(cells, i)
        {
	    //AU[cells[i]] += mu[cells[i]]*D + (rho[cells[i]]*mag(U[cells[i]]))*F;
	    AU[cells[i]] += ::pow(T[cells[i]], DTExp_)*mu[cells[i]]*D + (::pow(T[cells[i]], FTExp_)*rho[cells[i]]*mag(U[cells[i]]))*F;
        }
    }
}


// ************************************************************************* //
