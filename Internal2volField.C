/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2018 OpenFOAM Foundation
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

Application


Description


\*---------------------------------------------------------------------------*/
#include "fvCFD.H"
#include "OFstream.H"
#include "ReadFields.H" // for the define of class::IOobjectList and function::readFields

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    #include "removeCaseOptions.H"
    timeSelector::addOptions();
    argList::addOption
    (
        "fields",
        "list",
        "specify a list of volScalarFields::Internal to be processed. Eg, '(sprayCloud:hsTrans)' - "
        "regular expressions not currently supported"
    );
    argList::addOption
    (
        "vectorFields",
        "list",
        "specify a list of volVectorFields::Internal to be processed. Eg, '(sprayCloud:UTrans)' - "
        "regular expressions not currently supported"
    );
    #include "setRootCase.H"
    #include "createTime.H"

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    instantList timeDirs = timeSelector::select0(runTime, args);

    List<word> selectedScalarFields;
    if (args.optionFound("fields"))
    {
        args.optionLookup("fields")() >> selectedScalarFields;
    }
    List<word> selectedVectorFields;
    if (args.optionFound("vectorFields"))
    {
        args.optionLookup("vectorFields")() >> selectedVectorFields;
    }

    forAll(timeDirs, timeI)
    {
        runTime.setTime(timeDirs[timeI], timeI);
        #include "createMesh.H"

        // Maintain a stack of the stored objects to clear after executing
        LIFOStack<regIOobject*> storedObjects;
        // Read objects in time directory
        IOobjectList objects(mesh, runTime.timeName());
        // Read Fields
        readFields<volScalarField::Internal>(mesh, objects, selectedScalarFields, storedObjects);
        readFields<volVectorField::Internal>(mesh, objects, selectedVectorFields, storedObjects);

        Switch contamination = false;
        forAll (selectedScalarFields, fieldI)
        {
            if (!mesh.objectRegistry::foundObject<volScalarField::Internal>(selectedScalarFields[fieldI]))
            {
                contamination = true;
                Info<< selectedScalarFields[fieldI] << " not found" << endl;
            }
        }
        forAll (selectedVectorFields, fieldI)
        {
            if (!mesh.objectRegistry::foundObject<volVectorField::Internal>(selectedVectorFields[fieldI]))
            {
                contamination = true;
                Info<< selectedVectorFields[fieldI] << " not found" << endl;
            }
        }
        if (contamination)
        {
            while (!storedObjects.empty())
            {
                storedObjects.pop()->checkOut();
            }
            Info << "At least one of the Eulerian Internal fields is missing, "
                     << nl << "break current time loop!" << endl;
            continue;
        }

        PtrList<volScalarField::Internal> allScalarFields(selectedScalarFields.size());
        forAll(allScalarFields, fieldI)
        {
            allScalarFields.set(fieldI, mesh.objectRegistry::lookupObject<volScalarField::Internal>(selectedScalarFields[fieldI]) );
        }
        PtrList<volVectorField::Internal> allVectorFields(selectedVectorFields.size());
        forAll(allVectorFields, fieldI)
        {
            allVectorFields.set(fieldI, mesh.objectRegistry::lookupObject<volVectorField::Internal>(selectedVectorFields[fieldI]) );
        }

        for (auto Field : allScalarFields)
        {
            volScalarField field_out
            (
                IOobject
                (
                    Field.name()+"_dummy",
                    mesh.time().timeName(),
                    mesh,
                    IOobject::NO_READ,
                    IOobject::AUTO_WRITE
                ),
                mesh,
                dimensionedScalar(Field.dimensions(), 0.)
            );

            forAll(field_out, cellI)
            {
                field_out[cellI] = Field[cellI];
            }

            field_out.write();
            Info<<"I am writing this field for you: "<< Field.name()+"_dummy" << endl;
        }
        for (auto Field : allVectorFields)
        {
            volVectorField field_out
            (
                IOobject
                (
                    Field.name()+"_dummy",
                    mesh.time().timeName(),
                    mesh,
                    IOobject::NO_READ,
                    IOobject::AUTO_WRITE
                ),
                mesh,
                dimensionedVector(Field.dimensions(), Zero)
            );

            forAll(field_out, cellI)
            {
                field_out[cellI] = Field[cellI];
            }

            field_out.write();
            Info<<"I am writing this field for you: "<< Field.name()+"_dummy" << endl;
        }

        while (!storedObjects.empty())
        {
            storedObjects.pop()->checkOut();
        }
    }

    Info<<"Finished!"<<endl;

    return 0;
}


// ************************************************************************* //
