/*
=============================================================================
    corsikaIOreader is a tool to read CORSIKA eventio files
    Copyright (C) 2004, 2013, 2019 Gernot Maier and Henrike Fleischhack

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
=============================================================================*/
/*! \class VGrisu
    \brief write grisudet readable output of CORSIKA results

    coordinate transformations:
       - CORSIKA: x to north, y to west, z upwards ( z = 0 is at observation level) (azimuth counterclockwise)
       - kascade: x to east, y to south, z downwards (azimuth clockwise)

    uses some of Konrad Bernloehrs IACT routines (delivered with CORSIKA package)

    photon lines:
      - ID of photon emitting particle not know from CORSIKA -> always 0

    \author
         Gernot Maier

    \date
         26/04/04
*/

#include "VGrisu.h"

VGrisu::VGrisu( string iVersion, int id )
{
    degrad = 45. / atan( 1. );
    
    fVersion = iVersion;
    bSTDOUT = false;
    
    primID = 0;
    xoff = 0;
    yoff = 0;
    qeff = 1.;
    observation_height = 100.;
    atm_id = id;
    if( id >= 0 )
    {
        atmset_( &atm_id, &observation_height );
    }
    makeParticleMap();
}

/*!
    create grisu output file
    \param ofile name of grisu output file
*/
void VGrisu::setOutputfile( string ofile )
{
    if( ofile != "stdout" )
    {
        of_file.open( ofile.c_str() );
        if( !of_file )
        {
            cout << "VGrisu::setOutputfile: error opening outputfile: " << ofile << endl;
            exit( -1 );
        }
        of_file.setf( ios::fixed | ios::right );
        of_file.precision( 4 );
        of_file.width( 15 );
    }
    else
    {
        bSTDOUT = true;
        cout.setf( ios::fixed | ios::right );
        cout.precision( 4 );
        cout.width( 15 );
    }
}

/*!
   write some information about CORSIKA run intot the runheader
*/
void VGrisu::writeRunHeader( float* buf1, VCORSIKARunheader* f )
{
    if( !bSTDOUT )
    {
        of_file << "* HEADF  <-- Start of header flag" << endl;
        of_file << endl;
        of_file << "photon list created with " << fVersion << endl;
        of_file << endl;
        of_file << "       Photons generated by CORSIKA  (date: " << ( int )buf1[44] << ")" << endl;
        of_file << endl;
        of_file << "\t CORSIKA run number: " << ( int )buf1[43] << endl;
        of_file << "\t CORSIKA version: " << buf1[45] << endl;
        of_file << endl << endl;
        
        of_file << " TITLE OF RUN: " << endl;
        of_file << "\t\t\t Primary energy<min.,max.> TeV = ";
        of_file << buf1[58] / 1.E3 << "\t" << buf1[59] / 1.E3 << endl;
        of_file << "\t\t\t Slope of energy spectrum: " << buf1[57] << endl;
        of_file << "\t\t\t Type code for primary particle (CORSIKA ID) " << ( int )buf1[2] << endl;
        of_file << "PTYPE: " << ( int )buf1[2] << endl;
        
        if( particles.find( ( int )buf1[2] ) != particles.end() )
        {
            of_file << "\t\t\t Type code for primary particle (kascade ID) " << particles[( int )buf1[2]] << endl;
        }
        else
        {
            of_file << "\t\t\t Type code for primary particle (kascade ID) \t unknown particle (for kascade)" << endl;
        }
        of_file << "\t\t\t Primary zenith angle  (CORSIKA coord.): " << buf1[10] * degrad << endl;
        of_file << "\t\t\t Primary azimuth angle (CORSIKA coord.): " << buf1[11] * degrad << endl;
        float x = 0.;
        float az = buf1[11];
        transformCoord( az, x, x );
        of_file << "\t\t\t Primary zenith angle  (kascade coord.): " << buf1[10] * degrad << endl;
        of_file << "\t\t\t Primary azimuth angle (kascade coord.): " << az* degrad << endl;
        of_file << "\t\t\t Magnetic field (x/z): " << buf1[70] << "\t" <<  buf1[71] << endl;
        of_file << "\t\t\t Observation height [m]: " <<  buf1[47] * 0.01 << endl;
        of_file << "\t\t\t Energy cuts (hadr./muon/el./phot.) [GeV]: ";
        of_file << buf1[60] << "\t" << buf1[61] << "\t" << buf1[62] << "\t" << buf1[63] << endl;
        
        of_file << "CORSIKA RUN HEADER (START)" << endl;
        if( f )
        {
            f->printHeader( of_file );
        }
        of_file << "CORSIKA RUN HEADER (END)" << endl;
        
        of_file << endl;
        of_file << "* DATAF  <-- end of header flag" << endl;
        of_file << "R " << qeff << endl;
        // observation heigth in [m]
        of_file << "H " << observation_height << endl;
    }
    else
    {
        cout << "* HEADF  <-- Start of header flag" << endl;
        cout << endl;
        cout << "photon list created with " << fVersion << endl;
        cout << endl;
        cout << "       Photons generated by CORSIKA  (date: " << ( int )buf1[44] << " C)" << endl;
        cout << endl;
        cout << "\t CORSIKA run number: " << ( int )buf1[43] << endl;
        cout << "\t CORSIKA version: " << buf1[45] << endl;
        cout << endl << endl;
        
        cout << " TITLE OF RUN: " << endl;
        cout << "\t\t\t Primary energy<min.,max.> TeV = ";
        cout << buf1[58] / 1.E3 << "\t" << buf1[59] / 1.E3 << endl;
        cout << "\t\t\t Slope of energy spectrum: " << buf1[57] << endl;
        cout << "\t\t\t Type code for primary particle (CORSIKA ID) " << ( int )buf1[2] << endl;
        
        if( particles.find( ( int )buf1[2] ) != particles.end() )
        {
            cout << "\t\t\t Type code for primary particle (kascade ID) " << particles[( int )buf1[2]] << endl;
        }
        else
        {
            cout << "\t\t\t Type code for primary particle (kascade ID) \t unknown particle (for kascade)" << endl;
        }
        cout << "PTYPE: " << ( int )buf1[2] << endl;
        cout << "\t\t\t Primary zenith angle  (CORSIKA coord.): " << buf1[10] * degrad << endl;
        cout << "\t\t\t Primary azimuth angle (CORSIKA coord.): " << buf1[11] * degrad << endl;
        float x = 0.;
        float az = buf1[11];
        transformCoord( az, x, x );
        cout << "\t\t\t Primary zenith angle  (kascade coord.): " << buf1[10] * degrad << endl;
        cout << "\t\t\t Primary azimuth angle (kascade coord.): " << az* degrad << endl;
        cout << "\t\t\t Magnetic field (x/z): " << buf1[70] << "\t" <<  buf1[71] << endl;
        cout << "\t\t\t Observation height [m]: " <<  buf1[47] * 0.01 << endl;
        cout << "\t\t\t Energy cuts (hadr./muon/el./phot.) [GeV]: ";
        cout << buf1[60] << "\t" << buf1[61] << "\t" << buf1[62] << "\t" << buf1[63] << endl;
        cout << endl;
        
        cout << "CORSIKA RUN HEADER (START)" << endl;
        if( f )
        {
            f->printHeader( cout );
        }
        cout << "CORSIKA RUN HEADER (END)" << endl;
        
        cout << endl;
        cout << "* DATAF  <-- end of header flag" << endl;
        cout << "R " << qeff << endl;
        // observation heigth in [m]
        cout << "H " << observation_height << endl;
    }
}

/*!
   write shower line ("S")
   \param array  MC run information
*/
void VGrisu::writeEvent( telescope_array array, bool printMoreInfo )
{
    float phi = array.shower_sim.azimuth / degrad;
    float ze = ( 90. - array.shower_sim.altitude ) / degrad;
    float x = array.shower_sim.xcore;
    float y = array.shower_sim.ycore;
    
    xoff = x;
    yoff = y;
    
    transformCoord( phi, x, y );           // transform CORSIKA to GrIsu coordinates
    
    float dcos = sin( ze ) * cos( phi );
    if( fabs( dcos ) < 1.e-8 )
    {
        dcos = 0.;    // rounding error
    }
    float dsin = sin( ze ) * sin( phi );
    if( fabs( dsin ) < 1.e-8 )
    {
        dsin = 0.;    // rounding error
    }
    double ih = 100. * array.shower_sim.firstint;
    double thick = -1;
    if( printMoreInfo )
    {
        thick = thickx_( &ih ) / cos( ze );
    }
    
    if( !bSTDOUT )
    {
        of_file << "S";
        of_file << " " << setprecision( 7 ) << array.shower_sim.energy;         // energy in TeV
        of_file << " " << setprecision( 7 ) << x;
        of_file << " " << setprecision( 7 ) << y;
        of_file << " " << setprecision( 7 ) << dcos;
        of_file << " " << setprecision( 7 ) << dsin;
        of_file << " " << setprecision( 7 ) << array.shower_sim.firstint;
        of_file << " " << -1;
        of_file << " " << -1;
        of_file << " " << -1;
        of_file << endl;
    }
    else
    {
        cout << "S";
        cout << " " << setprecision( 7 ) << array.shower_sim.energy;         // energy in TeV
        cout << " " << setprecision( 7 ) << x;
        cout << " " << setprecision( 7 ) << y;
        cout << " " << setprecision( 7 ) << dcos;
        cout << " " << setprecision( 7 ) << dsin;
        cout << " " << setprecision( 7 ) << array.shower_sim.firstint;
        cout << " " << -1;
        cout << " " << -1;
        cout << " " << -1;
        cout << endl;
    }
    
    //additional corsika information in separate line. Format is "C", first interaction height, first interaction depth, corsika shower id.
    if( printMoreInfo )
    {
        if( !bSTDOUT )
        {
            of_file << "C " <<  setprecision( 7 ) << array.shower_sim.firstint  << " " << thick <<  " " << array.shower_sim.shower_id << endl;
        }
        else
        {
            cout <<  "C " <<  setprecision( 7 ) << array.shower_sim.firstint  << " " << thick <<  " " << array.shower_sim.shower_id << endl;
        }
        
    }
    
}

/*!
    write next photon to grisu file ("P" line)

    \param i_bunch photon information
    \param i_tel   telescope number

*/
void VGrisu::writePhotons( bunch i_bunch, int i_tel )
{

    float x = i_bunch.x;
    float y = i_bunch.y;
    float az = atan2( i_bunch.cy, i_bunch.cx );
    float ze = 1. - ( i_bunch.cx * i_bunch.cx + i_bunch.cy * i_bunch.cy );
    if( ze > 0. )
    {
        ze = sqrt( ze );
    }
    else
    {
        ze = 0.;
    }
    ze = acos( ze );
    
    transformCoord( az, x, y );
    
    if( !bSTDOUT )
    {
        of_file.setf( ios::showpos );
        of_file << "P" << " ";
        of_file << setprecision( 7 ) << x << " ";
        of_file << setprecision( 7 ) << y << " ";
        of_file << setprecision( 7 ) << sin( ze ) * cos( az ) << " ";
        of_file << setprecision( 7 ) << sin( ze ) * sin( az ) << " ";
        of_file << setprecision( 7 ) << i_bunch.zem << " ";
        of_file << setprecision( 7 ) << i_bunch.ctime << " ";             // !! not relative time since emission,
        // but time since first interaction
        of_file << ( int )i_bunch.lambda  << " " ;                        // in nanometer
        of_file << 3  << " ";                                             // the type of the particle emitting the photon,
        // (not know from CORSIKA)
        of_file << i_tel + 1;                                                   // the detector hit (negative integer number)
        of_file << endl;
        
        of_file.unsetf( ios::showpos );
    }
    else
    {
        cout.setf( ios::showpos );
        cout << "P" << " ";
        cout << setprecision( 7 ) << x << " ";
        cout << setprecision( 7 ) << y << " ";
        cout << setprecision( 7 ) << sin( ze ) * cos( az ) << " ";
        cout << setprecision( 7 ) << sin( ze ) * sin( az ) << " ";
        cout << setprecision( 7 ) << i_bunch.zem << " ";
        cout << setprecision( 7 ) << i_bunch.ctime << " ";             // !! not relative time since emission,
        // but time since first interaction
        cout << ( int )i_bunch.lambda  << " " ;                        // in nanometer
        cout << 3  << " ";                                             // the type of the particle emitting the photon,
        // (not know from CORSIKA)
        cout << i_tel + 1;                                                   // the detector hit (negative integer number)
        cout << endl;
        
        
        cout.unsetf( ios::showpos );
    }
}

/*!
    transform CORSIKA particle IDs to kascade particle IDs
*/
void VGrisu::makeParticleMap()
{
    particles.clear();
    
    particles[1] = 1; // gamma
    particles[2] = 2; // e
    particles[3] = 3; // pos
    particles[5] = 4; // mu+
    particles[6] = 5; // mu-
    particles[7] = 6; // pi0
    particles[8] = 7; // pi+
    particles[9] = 8; // pi-
    particles[11] = 9; // k+
    particles[12] = 10; // k-
    particles[10] = 11; // k0long
    particles[16] = 12; // k0short
    particles[14] = 13; // proton
    particles[13] = 14; // neutron
}

/*!
     transfrom from Corsika to grisu coordinates

     - angles in radiants
*/
void VGrisu::transformCoord( float& az, float& x, float& y )
{
    az = redang( 1.5 * M_PI - redang( az ) );
    
    float i_y = y;
    y = -1.* x;
    x = -1.* i_y;
}

//! reduce large angle to intervall 0, 2*pi
float VGrisu::redang( float iangle )
{
    if( iangle >= 0 )
    {
        iangle = iangle - int( iangle / ( 2. * M_PI ) ) * 2. * M_PI;
    }
    else
    {
        iangle = 2. * M_PI + iangle + int( iangle / ( 2. * M_PI ) ) * 2. * M_PI;
    }
    
    return iangle;
}
