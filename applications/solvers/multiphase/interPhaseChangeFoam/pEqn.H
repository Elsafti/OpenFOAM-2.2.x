{
    volScalarField rAU("rAU", 1.0/UEqn.A());
    surfaceScalarField rAUf("Dp", fvc::interpolate(rAU));

    volVectorField HbyA("HbyA", U);
    HbyA = rAU*UEqn.H();

    surfaceScalarField phiHbyA
    (
        "phiHbyA",
        (fvc::interpolate(HbyA) & mesh.Sf())
      + fvc::ddtPhiCorr(rAU, rho, U, phi)
    );
    adjustPhi(phiHbyA, U, p_rgh);
    phi = phiHbyA;

    surfaceScalarField phig
    (
        (
            fvc::interpolate(interface.sigmaK())*fvc::snGrad(alpha1)
          - ghf*fvc::snGrad(rho)
        )*rAUf*mesh.magSf()
    );

    phiHbyA += phig;

    Pair<tmp<volScalarField> > vDotP = twoPhaseProperties->vDotP();
    const volScalarField& vDotcP = vDotP[0]();
    const volScalarField& vDotvP = vDotP[1]();

    while (pimple.correctNonOrthogonal())
    {
        fvScalarMatrix p_rghEqn
        (
            fvc::div(phiHbyA) - fvm::laplacian(rAUf, p_rgh)
          - (vDotvP - vDotcP)*(pSat - rho*gh) + fvm::Sp(vDotvP - vDotcP, p_rgh)
        );

        p_rghEqn.setReference(pRefCell, pRefValue);

        p_rghEqn.solve(mesh.solver(p_rgh.select(pimple.finalInnerIter())));

        if (pimple.finalNonOrthogonalIter())
        {
            phi = phiHbyA + p_rghEqn.flux();

            U = HbyA + rAU*fvc::reconstruct((phig + p_rghEqn.flux())/rAUf);
            U.correctBoundaryConditions();
            fvOptions.correct(U);
        }
    }

    p == p_rgh + rho*gh;

    if (p_rgh.needReference())
    {
        p += dimensionedScalar
        (
            "p",
            p.dimensions(),
            pRefValue - getRefCellValue(p, pRefCell)
        );
        p_rgh = p - rho*gh;
    }
}
