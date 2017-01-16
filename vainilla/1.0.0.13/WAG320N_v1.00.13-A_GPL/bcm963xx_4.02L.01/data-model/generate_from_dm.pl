#!/usr/bin/perl -w 

#***********************************************************************
#
#  Copyright (c) 2007  Broadcom Corporation
#  All Rights Reserved
#
# 
# 
# This program is the proprietary software of Broadcom Corporation and/or its 
# licensors, and may only be used, duplicated, modified or distributed pursuant 
# to the terms and conditions of a separate, written license agreement executed 
# between you and Broadcom (an "Authorized License").  Except as set forth in 
# an Authorized License, Broadcom grants no license (express or implied), right 
# to use, or waiver of any kind with respect to the Software, and Broadcom 
# expressly reserves all rights in and to the Software and all intellectual 
# property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE 
# NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY 
# BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE. 
# 
# Except as expressly set forth in the Authorized License, 
# 
# 1. This program, including its structure, sequence and organization, 
#    constitutes the valuable trade secrets of Broadcom, and you shall use 
#    all reasonable efforts to protect the confidentiality thereof, and to 
#    use this information only in connection with your use of Broadcom 
#    integrated circuit products.  (Note this clause prohibits you from 
#    linking, either statically or dynamically, this program with any software 
#    that is licensed under the GPL, as the terms of the GPL would force you 
#    to release the source code of this program, thus violating the 
#    confidentiality aspect of this clause.) 
# 
# 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS" 
#    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR 
#    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH 
#    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND 
#    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, 
#    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR 
#    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE 
#    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR 
#    PERFORMANCE OF THE SOFTWARE. 
# 
# 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR 
#    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL, 
#    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY 
#    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN 
#    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; 
#    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE 
#    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS 
#    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY 
#    LIMITED REMEDY. 
#
#
#***********************************************************************/


#
# This script reads the cms-data-model in XML format.
# It then generates various .h and .c files that represent the data model in the
# c programming language that can be compiled into libmdm.so
# 
# Usage: See the usage function at the bottom of this file.
#
#

use strict;
use GenObjectNode;
use GenParamNode;
use Utils;


###########################################################################
#
# This first section contains helper functions.
#
###########################################################################


my $spreadsheet1Done = 0;
my $savedRowHashValid = 0;
my %savedRowHash;
my $firstValidOid = 1;
my $gOid = $firstValidOid;



my $IGDRootObjRef;
my %profileToValidStringsHash;
my %profileToFileHandleHash;
my %allValidStrings;
my %uniqueProfiles;
my @allVSArefs;



sub parse_object_line
{
    my ($hashref, $line) = @_;

#    print "parsing object line: $line";

    ${$hashref}{type} = "object";

    $line =~ /name="([\w.{}]+)"/;
    ${$hashref}{name} = $1;

    $line =~ /shortObjectName="([\w]+)"/;
    ${$hashref}{shortObjectName} = $1;

    $line =~ /specSource="([\w]+)"/;
    ${$hashref}{specSource} = $1;

    $line =~ /profile="([\w:]+)"/;
    ${$hashref}{profile} = $1;
    if (!(defined($1))) {
       die "could not extract profile for $line";
    }

    $line =~ /requirements="([\w]+)"/;
    ${$hashref}{requirements} = $1;

    $line =~ /supportLevel="([\w]+)"/;
    ${$hashref}{supportLevel} = $1;

    if ($line =~ /pruneWriteToConfigFile="([\w]+)"/)
    {
        ${$hashref}{pruneWriteToConfigFile} = $1;
    }
    else
    {
        ${$hashref}{pruneWriteToConfigFile} = "false";
    }
}

sub parse_parameter_line
{
    my ($hashref, $line) = @_;

#    print "parsing param line: $line";

#    undef ${$hashref}{requirements};
#    undef ${$hashref}{minValue};
#    undef ${$hashref}{maxValue};
#    undef ${$hashref}{maxLength};
#    undef ${$hashref}{validValuesArray};

    # remove all entries from hashref
    %{$hashref} = ();


    $line =~ /name="([\w.{}]+)"/;
    ${$hashref}{name} = $1;

    $line =~ /type="([\w]+)"/;
    ${$hashref}{type} = $1;

    $line =~ /specSource="([\w]+)"/;
    ${$hashref}{specSource} = $1;

    $line =~ /profile="([\w:]+)"/;
    ${$hashref}{profile} = $1;

    if ($line =~ /requirements="([\w]+)"/)
    {
        ${$hashref}{requirements} = $1;
    }

    $line =~ /supportLevel="([\w]+)"/;
    ${$hashref}{supportLevel} = $1;

    if ($line =~ /denyActiveNotification="([\w]+)"/)
    {
        ${$hashref}{denyActiveNotification} = $1;
    }

    if ($line =~ /forcedActiveNotification="([\w]+)"/)
    {
        ${$hashref}{forcedActiveNotification} = $1;
    }

    if ($line =~ /alwaysWriteToConfigFile="([\w]+)"/)
    {
        ${$hashref}{alwaysWriteToConfigFile} = $1;
    }

    if ($line =~ /neverWriteToConfigFile="([\w]+)"/)
    {
        ${$hashref}{neverWriteToConfigFile} = $1;
    }

    if ($line =~ /transferDataBuffer="([\w]+)"/)
    {
        ${$hashref}{transferDataBuffer} = $1;
    }

    if ($line =~ /maxLength="([\d]+)"/)
    {
        ${$hashref}{maxLength} = $1;
    }

    if ($line =~ /minValue="([\d-]+)"/)
    {
        ${$hashref}{minValue} = $1;
    }

    if ($line =~ /maxValue="([\d-]+)"/)
    {
        ${$hashref}{maxValue} = $1;
    }

    if ($line =~ /validValuesArray="([\w]+)"/)
    {
        ${$hashref}{validValuesArray} = $1;
    }

    if ($line =~ /defaultValue="([\w\s.\/:,\(\)=-]+)"/)
    {
        ${$hashref}{defaultValue} = $1;
    }
    else
    {
        # we must have a defaultValue, so just set to NULL
        ${$hashref}{defaultValue} = "NULL";
    }
}


#
# Parse a line in the data model file.
# Fill in the given reference to a hash with attributes found in the line.
#
sub parse_row
{
    my ($hashref) = $_[0];
    my $i;
    my $endRowMarkerFound=0;
    my $gotLine=0;

#    print "starting parse_row (spreadsheet1Done=$spreadsheet1Done) \n";
    if ($spreadsheet1Done == 1)
    {
        return 0;
    }

    while ($gotLine == 0)
    {
        $_ = <STDIN>;

        if (/[\w]*<vsaInfo>/)
        {
#            print "Begin vsaInfo tag found, end parse_row\n";
            $spreadsheet1Done = 1;
            return 0;
        }

        
        if (!defined($_)) {
            print "end of file detected\n";
            return 0;
        }

#        if (/[\w]*<\/xmlMandatorySingleRootNode>/) {
#            print "end tag detected $_";
#            return 0;
#        }

        # We are only interested in rows that begin with object
        # or parameter.
        if (/[\w]*<object/)
        {
            parse_object_line($hashref, $_);
            return 1;
        }

        if (/[\w]*<parameter/)
        {
            parse_parameter_line($hashref, $_);
            return 1;
        }
    }


#
# These verify that all fields needed to generate the MDM tree
# can be read in.
#
#  print "${$hashref}{\"Name\"} ${$hashref}{\"Type\"} ${$hashref}{\"ValidatorData\"} ";
#  print "${$hashref}{\"DefaultValue\"} ${$hashref}{\"BcmInitialValue\"} ${$hashref}{\"BcmSuggestedValue\"} ";
#  print "${$hashref}{\"TRxProfile\"} ${$hashref}{\"NotificationFlags\"} ${$hashref}{\"BcmFlags\"} ";
#  print "${$hashref}{\"BcmAbbreviatedName\"} \n";

    die "impossible!  reached end of parse_row";
}


#
# Parse a row in the Valid Values spreadsheet.
# The alorgithm is slightly different than the one
# used to parse the MDM spreadsheet
#
sub parse_row_vv
{
    my $hashRef = $_[0];
    my $rowMarkerFound = 0;
    my ($currData, $currProfile);
    my $gotLine=0;

    %{$hashRef} = ();

#    print "starting parse_row_vv\n";

    while ($gotLine == 0)
    {
        $_ = <STDIN>;

        if (/[\w]*<\/vsaInfo/)
        {
#            print "End vsaInfo tag found.\n";
            return 0;
        }

        if (!defined($_)) {
            print "end of file detected\n";
            return 0;
        }

        if (/[\w]*<validstringarray/)
        {
            /name="([\w]+)"/;
            ${$hashRef}{"name"} = $1;
            ${$hashRef}{"profile"} = "Baseline";
#            print "Got vsa=${$hashRef}{name}\n";
            return 1;
        }

        if (/[\w]*<element/)
        {
            /element>([\w\s._-]+)<\/element/;
            ${$hashRef}{"name"} = $1;
#            print "     name=${$hashRef}{name}\n";
            return 1;
        }
    }

    die "impossible!  reached end of parse_row_vv";
}


#
# If no short object name was supplied, then
# just clean up the fully qualified (generic) object name
# and make that the MDMOBJID_name
#
sub convert_fullyQualifiedObjectName
{
    my $name = $_[0];

#    print "Converting $name\n";

    # get rid of trailing dot
    $name =~ s/\.$//;

    # get rid of trailing instance id specifier
    $name =~ s/\.{i}$//;

    # convert all dots ('.') or instance specifiers
    # ('.{i}.') into underscores ('_')
    $name =~ s/\.{i}\./_/g;
    $name =~ s/\./_/g;

    # convert all letters to upper case
    $name =~ tr/[a-z]/[A-Z]/;

    # if there is a _INTERNETGATEWAYDEVICE_ in the middle
    # of the object name, delete it.  Its redundant.
    $name =~ s/INTERNETGATEWAYDEVICE_//;

    return $name;
}



#
# Convert a short object name from the Data Model spreadsheet
# to the "all caps with underscore separator" form.
#
sub convert_shortObjectName
{
    my $name = $_[0];
    my ($line, $nextWord, $restOfLine);

    #
    # Strip the trailing word "Object".  All of our abbreviated
    # object names end with object.
    #
    $name =~ s/Object$//;

    #
    # check for special case where the entire abbreviated name
    # is all caps, e.g. IGD.  If so, just return that.
    #
    if (!($name =~ /[a-z]+/))
    {
        return $name;
    }

    #
    # Check for word that starts with multiple capitol letters,
    # e.g. PPPLinkConfig
    #

    $name =~ /([A-Z])([A-Z]+)([A-Z])([\w]*)/;

    if (defined($2))
    {
#        print "Multi cap found, 3=$3 restOfLine=$4\n";
        $line = $1 . $2;
        $restOfLine = $3 . $4;
    }
    else
    {
        $line = "";
        $restOfLine = $name;
    }

#    print "After all caps processing, $line <-> $restOfLine\n";

    while (!($restOfLine =~ /^[A-Z][a-z0-9]*$/))
    {
        $restOfLine =~ /([A-Z][a-z0-9]*)([A-Z][\w\d]*)/;
        $nextWord = $1;
        $restOfLine = $2;

#        print "nextWord=$nextWord restOfLine=$restOfLine\n";

        $nextWord =~ tr/[a-z]/[A-Z]/;
        if ($line eq "")
        {
            $line = $nextWord;
        }
        else
        {
            $line = $line . "_" . $nextWord;
        }

#        print "line is now $line\n";
#        print "restOfLine = $restOfLine\n";
    }


    # this is the last word
#    print "last word $restOfLine\n";

    $restOfLine =~ tr/[a-z]/[A-Z]/;
    if ($line eq "")
    {
        $line = $restOfLine;
    }
    else
    {
        $line = $line . "_" . $restOfLine;
    }

    return $line;
}



#
# Turn the type name from TR98 format to our format
#
sub convert_typeName
{
    my $typeName = $_[0];

    if ($typeName =~ /string/i)
    {
        $typeName = "char *   ";
    }
    elsif ($typeName =~ /unsignedInt/i)
    {
        $typeName = "UINT32   ";
    }
    elsif ($typeName =~ /int/i)
    {
        $typeName = "SINT32   ";
    }
    elsif ($typeName =~ /boolean/i)
    {
        $typeName = "UBOOL8   ";
    }
    elsif ($typeName =~ /base64/i)
    {
        $typeName = "BASE64   ";
    }
    elsif ($typeName =~ /dateTime/i)
    {
        $typeName = "DATETIME ";
    }

    return $typeName;
}


#
# Convert the first letter of a leaf parameter name that is
# to be used as a field name in a MdmObject structure to
# lower case.  However, don't lower case the first letter
# if it is part of an all capitol acronym, e.g. ATMPeakCellRate
#
sub convert_fieldName
{
    my ($firstLetter, $secondLetter, $restOfLine);

    $_ = $_[0];
    /([A-Z])([\w])([\w]*)/;
    $firstLetter = $1;
    $secondLetter = $2;
    $restOfLine = $3;

    if ($secondLetter =~ /[a-z]/)
    {
        $firstLetter =~ tr/[A-Z]/[a-z]/;
        return $firstLetter . $secondLetter . $restOfLine;
    }
    else
    {
        return $_[0];
    }
}

#
# Change the valid value string to all caps.
# Change space and '.' to _
#
sub convert_validstring
{
    my $str = $_[0];

    $str =~ tr/[ ]/[_]/;
    $str =~ tr/[\.]/[_]/;
    $str =~ tr/[\-]/[_]/;

    $str =~ tr/[a-z]/[A-Z]/;

    return $str;
}


#
# Change the profile name to a filename
#
sub convert_profileNameToFilename
{
    my $str = $_[0];

    $str =~ tr/[:]/[_]/;
    
    $str = $str . ".c";

    return $str;
}


#
# Lower case the first word, with special case exceptions.
# Works similar to the standard perl function lcfirst
#
sub getLowerCaseFirstLetterSpecial
{
    my ($orig) = @_;
    my $lc;

    if ($orig =~ /^IGD([\w]+)/)
    {
        $lc = "igd" . $1;
    }
    elsif ($orig =~ /^IP([\w]+)/)
    {
        $lc = "ip" . $1;
    }
    else
    {
        $lc = lcfirst $orig;
    }

    return $lc;
}


#
# return the depth of the current path by counting the number
# of "." in the path.  Each "." counts as depth 1, except when
# there is ".{i}.", which only counts as 1.
#
# So InternetGatewayDevice.           counts as 1
#    InternetGatewayDevice.DeviceInfo. counts as 2
#    InternetGatewayDevice.DeviceInfo.VendorConfigFile.{i}. counts as 3
#
sub get_pathDepth
{
    my $restOfLine = $_[0];
    my $depth = 1;

    chomp($restOfLine);

#    print "get_pathDepth=$restOfLine\n";

    while ($restOfLine =~ /([\w]+)\.([\w{}\.]+)/)
    {
        $restOfLine = $2;
#        print "($depth) 1=$1 2=$2\n";

        # Get rid of intermediate "{i}." in the path
        if ($restOfLine =~ /^{i}\.([\w{}\.]+)/)
        {
            $restOfLine = $1;
        }

        $depth += 1;

        if ($restOfLine =~ /^[\w]+\.{i}\.$/)
        {
            # restOfLine consists entirely of "name.{i}." so we are done.
            last;
        }

        if ($depth > 10)
        {
            die "impossible depth";
        }
    }

    return $depth;
}


#
# Open a filehandle and return a reference to it.
# This is useful when we want to output the MDM tree to multiple files.
# See Chapter 2, page 51, of Programming Perl
#
sub open_filehandle
{
    my $filename = $_[0];
    my $overWriteFilename = ">" . $filename;
    local *FH;

    open (FH, $overWriteFilename) || die "Could not open $filename";

    return *FH;
}


#
# return the filehandle associated with this profile for the purpose
# of outputing the mdm tree.  If the filehandle for the specified
# profile has not been opened yet, open it and inject the autogen_warning.
#
sub get_profiled_filehandle
{
    my ($basedir, $profile) = @_;
    my $fileRef;
    my $def;

    $fileRef = $profileToFileHandleHash{$profile};
    if (!defined($fileRef))
    {
        my $filename;
        my $converted_profile_name;
        $converted_profile_name = convert_profileNameToFilename($profile);

        $filename = $basedir . "/" . $converted_profile_name;
#        print "new profile, filename= $filename\n";
        $fileRef = open_filehandle($filename);
        $profileToFileHandleHash{$profile} = $fileRef;
        autogen_warning($fileRef);

        $def = convertProfileNameToPoundDefine($profile);
        print $fileRef "#ifdef $def\n\n";
    }

    return $fileRef;
}

sub close_all_profiled_filehandles
{
    my $key;
    my $fileRef;

    foreach $key (keys (%profileToFileHandleHash))
    {
        $fileRef = $profileToFileHandleHash{$key};
        print $fileRef "\n\n#endif\n";
        close $profileToFileHandleHash{$key};
    }
}



###########################################################################
#
# Begin of top level function for generating a .h or .c file
# The first couple of output_xxx functions are pretty simple, they
# process the cms-data-model.xml from stdin in a single pass.
# Towards the end of this section, we have more complicated functions
# that suck in the cms-data-model.xml and builds an internal data
# structure tree, and then ouputs the desired files by recursively
# traversing the data structure.
#
###########################################################################


#
# Top Level function for creating MdmObjectId's
#
sub output_mdmObjectIdFile
{
    my $fileRef = shift;
    my %rowHash;
    my $objName;
    my $objid=$firstValidOid;

    while (parse_row(\%rowHash))
    {
        if (($rowHash{"type"} =~ /object/i) && ($rowHash{"supportLevel"} ne "NotSupported"))
        {
            if ($rowHash{"shortObjectName"} =~ /None/i)
            {
                $objName = convert_fullyQualifiedObjectName($rowHash{"name"});
            }
            else
            {
                $objName = convert_shortObjectName($rowHash{"shortObjectName"});
            }

            print $fileRef "/*! \\brief $rowHash{\"name\"} */\n";
            print $fileRef "#define MDMOID_$objName  $objid\n\n";
            $objid += 1;
        }
    }

    $objid -= 1;
    print $fileRef "/*! \\brief maximum OID value */\n";
    print $fileRef "#define MDM_MAX_OID $objid\n\n";
}


#
# Top Level function for creating MdmObjectId string table
#
sub output_mdmOidStringTable
{
    my $fileRef = shift;
    my %rowHash;
    my $objName;
    my $objid=$firstValidOid;

    print $fileRef "const char *oidStringTable[]= { \n";
    printf $fileRef ("  /* %3d */ \"(invalid-0)\",\n", 0);

    while (parse_row(\%rowHash))
    {
        if (($rowHash{"type"} =~ /object/i) && ($rowHash{"supportLevel"} ne "NotSupported"))
        {
            if ($objid != $firstValidOid)
            {
                print $fileRef ",\n";
            }
            printf $fileRef ("  /* %3d */ \"%s\"", $objid, $rowHash{"name"});
            $objid += 1;
        }
    }

    print $fileRef "\n};\n";
}


#
# Top Level function for creating table of oid to handler functions pointers.
#
sub output_mdmOidHandlerFuncsTable
{
    my $fileRef = shift;
    my %rowHash;
    my $objName;
    my $objid=$firstValidOid;
    my ($rclfunc, $stlfunc);
    my $inVoiceRegion = 0;
    my $depth;
    my $currDepth = 0;
    my @profileStack;
    my $i = 0;
    my $conditionalLine;
    my $profileDef;

    # InternetGatewayDevice is at depth 1, so initialize depth 0 with something.
    $profileStack[0] = "Unspecified";

    print $fileRef "#include \"mdm_types.h\"\n";
    print $fileRef "#include \"rcl.h\"\n";
    print $fileRef "#include \"stl.h\"\n\n";

    print $fileRef "const MdmHandlerFuncInfo oidHandlerFuncsTable[]= { \n";
    printf $fileRef ("  /* %3d */ {NULL, NULL},\n", 0);

    while (parse_row(\%rowHash))
    {
        if (($rowHash{"type"} =~ /object/i) && ($rowHash{"supportLevel"} ne "NotSupported"))
        {

            $objName = getLowerCaseFirstLetterSpecial($rowHash{"shortObjectName"});
            $rclfunc = "rcl_" . $objName ;
            $stlfunc = "stl_" . $objName ;

            $depth = get_pathDepth($rowHash{name});
            if ($depth > $currDepth)
            {
                # increasing depth, push another profile on the stack
                push(@profileStack, $rowHash{profile});
                $currDepth = $depth;
            }
            elsif ($depth == $currDepth)
            {
                # we are at the same depth, just update the profile name at the current depth
                $profileStack[$depth] = $rowHash{profile};
            }
            else
            {
                # we are decreasing in depth.  We could decrease by more than one level,
                # so be sure to pop the right number of elements from the stack.
                while ($depth < $currDepth)
                {
                    pop(@profileStack);
                    $currDepth--;
                }
                # now we are at the same depth, update the profile name at the current depth
                $profileStack[$depth] = $rowHash{profile};
            }

            $conditionalLine = "";
            for ($i = 0; $i <= $currDepth; $i++)
            {
                if (($profileStack[$i] ne "Unspecified") &&
                    ($profileStack[$i] ne "Baseline:1") &&
                    ($profileStack[$i] ne "X_BROADCOM_COM_Baseline:1"))
                {
                    $profileDef = convertProfileNameToPoundDefine($profileStack[$i]);
                    if ($conditionalLine eq "")
                    {
                        $conditionalLine = "defined($profileDef)";
                    }
                    else
                    {
                        $conditionalLine = $conditionalLine . " && defined($profileDef)";
                    }
                }
            }

            if ($conditionalLine eq "")
            {
                # no special profile bracketing needed.
                printf $fileRef ("  /* %3d */ {$rclfunc, $stlfunc},\n", $objid);
            }
            else
            {
                # bracket rcl/stl handler functions under ifdef
                printf $fileRef ("#if $conditionalLine\n");
                printf $fileRef ("  /* %3d */ {$rclfunc, $stlfunc},\n", $objid);
                printf $fileRef ("#else\n");
                printf $fileRef ("  /* %3d */ {NULL, NULL},\n", $objid);
                printf $fileRef ("#endif\n");
            }

            $objid += 1;
        }
    }

    print $fileRef "\n};\n";
}


#
# Top Level function for creating MdmObject structure definitions
#
sub output_mdmObjectFile
{
    my $fileRef = shift;
    my (%rowHash, %prevObjRowHash);
    my $firstRow=1;
    my $objid = $firstValidOid;
    my $oidName;
    my $printParams=1;


    while (parse_row(\%rowHash))
    {
        if ($rowHash{"type"} =~ /object/i)
        {

            if ($rowHash{"shortObjectName"} =~ /None/i)
            {
                $oidName = convert_fullyQualifiedObjectName($rowHash{"name"});
            }
            else
            {
                $oidName = convert_shortObjectName($rowHash{"shortObjectName"});
            }


            if ($firstRow == 1)
            {
                %prevObjRowHash = %rowHash;
                $firstRow = 0;

                # The first object is always InternetGatewayDevice, and it will
                # always be supported, so no need to check for that here.

                print $fileRef "/*! \\brief Obj struct for $rowHash{\"name\"}\n";
                print $fileRef " *\n";
                print $fileRef " * MDMOID_$oidName $objid\n";
                print $fileRef " */\n";
                print $fileRef "typedef struct\n{\n";
                print $fileRef "    MdmObjectId _oid;\t/**< for internal use only*/\n";

                $objid += 1;
            }
            else
            {
                my $objName;

                if ($prevObjRowHash{"supportLevel"} ne "NotSupported")
                {
                    if ($prevObjRowHash{"shortObjectName"} =~ /None/i)
                    {
                        $objName = convert_fullyQualifiedObjectName($prevObjRowHash{"name"});
                        $objName = $objName . "_OBJECT";
                    }
                    else
                    {
                        $objName = $prevObjRowHash{"shortObjectName"};
                    }


                    print $fileRef "} $objName;\n\n";

                    print $fileRef "/*! \\brief _$objName is used internally to represent $objName */\n";
                    print $fileRef "typedef $objName _$objName;\n\n\n\n";
                }

                if ($rowHash{"supportLevel"} ne "NotSupported")
                {
                    print $fileRef "/*! \\brief Obj struct for $rowHash{\"name\"}\n";
                    print $fileRef " *\n";
                    print $fileRef " * MDMOID_$oidName $objid\n";
                    print $fileRef " */\n";
                    print $fileRef "typedef struct\n{\n";
                    print $fileRef "    MdmObjectId _oid;\t/**< for internal use only */\n";

                    $objid += 1;
                    $printParams = 1;
                }
                else
                {
                    # the current object is not supported, don't print out any
                    # of its params either.
                    $printParams = 0;
                }

                %prevObjRowHash = %rowHash;
            }

        }
        else
        {
            #
            # Define fields in the structure body.
            # Change the type name to something we define.
            # Make the first letter of the variable name lower case.
            # Surround the parameter in #ifdef profile if the parameter's
            # profile is different than that of parent object profile.
            #
            if ($printParams == 1)
            {
                my $convertedTypeName = convert_typeName($rowHash{"type"});
                my $convertedFieldName = convert_fieldName($rowHash{"name"});
                my $def;
                my $printEndif=0;

                if ($rowHash{"supportLevel"} ne "NotSupported")
                {
                    if (($prevObjRowHash{"profile"} ne $rowHash{"profile"}) &&
                        !($rowHash{"profile"} =~ /unspecified/i))
                    {
                        $def = Utils::convertProfileNameToPoundDefine($rowHash{"profile"});
                        print $fileRef "#ifdef $def\n";
                        $printEndif = 1;
                    }

                    print $fileRef "    $convertedTypeName $convertedFieldName;";
                    print $fileRef "\t/**< $rowHash{\"name\"} */\n";

                    if ($printEndif == 1)
                    {
                        print $fileRef "#endif\n";
                    }
                }
            }
        }
    }


    #
    # We don't get a last "object" type, but we
    # know to end the struct definition when the
    # end of file is reached.
    #
    if ($prevObjRowHash{"supportLevel"} ne "NotSupported")
    {
        my $objName;

        if ($prevObjRowHash{"shortObjectName"} =~ /None/i)
        {
            $objName = convert_fullyQualifiedObjectName($prevObjRowHash{"name"});
            $objName = $objName . "_OBJECT";
            print $fileRef "} $objName;\n\n";
        }
        else
        {
            $objName = $prevObjRowHash{"shortObjectName"};
        }

        print $fileRef "} $objName;\n\n";

        print $fileRef "/*! \\brief _$objName is used internally to represent $objName */\n";
        print $fileRef "typedef $objName _$objName;\n\n\n\n";
    }
}


#
# Top Level function for creating RCL and STL prototypes
#
sub output_prototypes
{
    my ($fileRef1, $fileRef2) = @_;
    my %rowHash;
    my $objName;
    my ($rclfunc, $stlfunc);

    while (parse_row(\%rowHash))
    {
        if (($rowHash{"type"} =~ /object/i) && ($rowHash{"supportLevel"} ne "NotSupported"))
        {
            $objName = getLowerCaseFirstLetterSpecial($rowHash{"shortObjectName"});

            $rclfunc = "rcl_" . $objName . "(";
            $stlfunc = "stl_" . $objName . "(" . "_$rowHash{\"shortObjectName\"}";

            print $fileRef1 "CmsRet $rclfunc _$rowHash{\"shortObjectName\"} *newObj,\n";
            print $fileRef1 "                const _$rowHash{\"shortObjectName\"} *currObj,\n";
            print $fileRef1 "                const InstanceIdStack *iidStack,\n";
            print $fileRef1 "                char **errorParam,\n";
            print $fileRef1 "                CmsRet *errorCode);\n\n";

            print $fileRef2 "CmsRet $stlfunc *obj, const InstanceIdStack *iidStack);\n\n";
        }
    }
}


#
# Top Level function for creating RCL and STL skeletons
#
sub output_skeletons
{
    my ($fileRef1, $fileRef2) = @_;
    my %rowHash;
    my $objName;
    my ($rclfunc, $stlfunc);

    while (parse_row(\%rowHash))
    {
        if (($rowHash{"Type"} =~ /object/i) && ($rowHash{"supportLevel"} ne "NotSupported"))
        {
            $objName = getLowerCaseFirstLetterSpecial($rowHash{"shortObjectName"});

            $rclfunc = "rcl_" . $objName . "(";
            $stlfunc = "stl_" . $objName . "(" . "_$rowHash{\"shortObjectName\"}";

            print $fileRef1 "CmsRet $rclfunc _$rowHash{\"shortObjectName\"} *newObj __attribute__((unused)),\n";
            print $fileRef1 "                const _$rowHash{\"shortObjectName\"} *currObj __attribute__((unused)),\n";
            print $fileRef1 "                const InstanceIdStack *iidStack __attribute__((unused)),\n";
            print $fileRef1 "                char **errorParam __attribute__((unused)),\n";
            print $fileRef1 "                CmsRet *errorCode __attribute__((unused)))\n";
            print $fileRef1 "{\nreturn CMSRET_SUCCESS;\n}\n\n";


            print $fileRef2 "CmsRet $stlfunc *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))\n";
            print $fileRef2 "{\nreturn CMSRET_SUCCESS;\n}\n\n";
        }
    }
}



#
# Read in entire data model xml file and build internal perl data structures
# and objects.
#
sub input_spreadsheet1
{
    my $currDepth = 0;

    input_nodes_recursively($IGDRootObjRef, $currDepth);

#    my $count = $IGDRootObjRef->getChildObjectCount();
#    print "Total objects directly under IGD=$count\n";
}


# spreadsheet2 just has valid values
sub input_spreadsheet2
{
    my $line;
    my %rowHash;
    my ($arrayRef, $profileArrayRef);
    my $dumpArray = 0;

    while (parse_row_vv(\%rowHash))
    {
        if (defined($rowHash{"profile"}))
        {
            # As debugging, print out the last fully populated values array
            if ($dumpArray && defined($arrayRef))
            {
                my $j=0;
                while (defined(${$arrayRef}[$j]))
                {
#                    print "${$arrayRef}[$j] ";
                    $j++;
                }
                print "\n";
            }
                
            # new set of valid values
            $arrayRef = [];
            ${$arrayRef}[0] = $rowHash{"name"};

            @allVSArefs = (@allVSArefs, $arrayRef);
        }
        else
        {
            # continuation of current set of valid values
            # Append to the current arrayRef
            @{$arrayRef} = (@{$arrayRef}, $rowHash{"name"});

            # This is for generating the validstrings file
            {
                my $nn;
                $nn = $rowHash{"name"};
                $allValidStrings{$nn} = 1;
            }
        }
    }
}


#
# Scan the MDM data model spreadsheet and input the object
# and param node information.
#
sub input_nodes_recursively
{
    my ($currObjRef, $currDepth) = @_;
    my $lastObjRef;
    my $addToObjRef = $currObjRef;


    #
    # If there is not a saved rowHash, then get the next row
    #
    while ($savedRowHashValid || parse_row(\%savedRowHash))
    {
        if ($savedRowHash{"type"} =~ /object/)
        {
            my $tmpObjRef;
            my $tmpObjDepth;
            my $tmpOid;

            $savedRowHashValid = 1;

            $tmpObjDepth = get_pathDepth($savedRowHash{"name"});

#            print "processing $savedRowHash{name} $currDepth $tmpObjDepth\n";

            if ($tmpObjDepth == $currDepth + 1)
            {
                # child object is at the right level, must be my immediate
                # child, so add it as my child.

                if ($savedRowHash{"supportLevel"} ne "NotSupported")
                {
                    $tmpOid = $gOid;
                    $gOid++;
                }
                else
                {
                    # this object is not supported.  Create the object anyways.
                    # (I suppose I could also simply not insert the object in 
                    # the array, but I don't want to mess with this fragile code).
                    # Don't increment the $gOid.  Pass in an invalid oid.
                    $tmpOid = 0;
                }

                $tmpObjRef = GenObjectNode::new GenObjectNode;
                $tmpObjRef->fillObjectInfo($tmpOid,
                                           $tmpObjDepth,
                                           $savedRowHash{"name"},
                                           $savedRowHash{"profile"},
                                           $savedRowHash{"supportLevel"},
                                           $savedRowHash{"shortObjectName"},
                                           $savedRowHash{"pruneWriteToConfigFile"});
                $lastObjRef = $tmpObjRef;

                # special case processing for the first node
                if ($savedRowHash{"name"} eq "InternetGatewayDevice.")
                {
                    $IGDRootObjRef = $tmpObjRef;
                    $addToObjRef = $tmpObjRef;
                }
                else
                {
                    $currObjRef->addChildObject($gOid, $savedRowHash{"name"}, $tmpObjRef);
                    $addToObjRef = $tmpObjRef;
                }

                $savedRowHashValid = 0;
            }
            elsif ($tmpObjDepth > $currDepth + 1)
            {
                # current child object has another child, recurse.
#               print "push down, next depth = $currDepth + 1\n";
                input_nodes_recursively($lastObjRef, $currDepth + 1);
            }
            else
            {
               # current object is actually not a child of me, pop out
               # of my recursion.
#               print "popping up, currDepth=$currDepth\n";
               last;
            }
        }
        else
        {
            # we are dealing with a parameter node which is under
            # the current object node.
            my $tmpParamRef;

            $tmpParamRef = GenParamNode::new GenParamNode;
            $tmpParamRef->fillParamInfo($savedRowHash{"name"},
                                        $savedRowHash{"type"},
                                        $savedRowHash{"defaultValue"},
                                        $savedRowHash{"profile"},
                                        $savedRowHash{"denyActiveNotification"},
                                        $savedRowHash{"mandatoryActiveNotification"},
                                        $savedRowHash{"alwaysWriteToConfigFile"},
                                        $savedRowHash{"neverWriteToConfigFile"},
                                        $savedRowHash{"transferDataBuffer"},
                                        $savedRowHash{"supportLevel"});

            if (defined($savedRowHash{"minValue"}))
            {
                if (!defined($savedRowHash{"maxValue"}))
                {
                    die "minValue is defined, but maxValue is not for $savedRowHash{name}\n";
                }

                $tmpParamRef->setMinMaxValues(${savedRowHash}{"minValue"},
                                              ${savedRowHash}{"maxValue"});
            }

            if (defined($savedRowHash{"maxValue"}))
            {
                if (!defined($savedRowHash{"minValue"}))
                {
                    die "maxValue is defined, but minValue is not for $savedRowHash{name}\n";
                }
            }

            if (defined($savedRowHash{"maxLength"}))
            {
                $tmpParamRef->setMaxLength(${savedRowHash}{"maxLength"});
            }

            if (defined($savedRowHash{"validValuesArray"}))
            {
                $tmpParamRef->setValidValuesArray(${savedRowHash}{"validValuesArray"});
            }


            if (!defined($addToObjRef)) {
                die "currobjref not defined.";
            }

            if (!defined($tmpParamRef)) {
                die "currobjref not defined.";
            }

            $addToObjRef->addParamNode($tmpParamRef);
        }
    }
}


#
# Output the valid string arrays bracked by #ifdef profile name
#
sub output_profiled_validstrings
{
    my $basedir = shift;
    my $fileRef;
    my ($key, $profileStr);
    my ($arrayRef, $arrayRef2, $vvRef);

#    print "starting output_profiled_validstrings\n";

    $fileRef = open_filehandle("$basedir/validstrings.c");

    autogen_warning($fileRef);

        # each entry the arrayRef is another arrayRefs to the valid values array
        foreach $vvRef (@allVSArefs)
        {
            my $firstEntry=1;
            my ($vvStr, $tmpStr);

            foreach $vvStr (@{$vvRef})
            {
                if ($firstEntry == 1)
                {
                    $tmpStr = $vvStr . "[]";
                    print $fileRef "const char * $tmpStr = {\n";
                    $firstEntry = 0;
                }
                else
                {
                    print $fileRef "\"$vvStr\",\n";
                }
            }
            print $fileRef "NULL\n";
            print $fileRef "};\n\n";
        }


    close $fileRef;
}


#
# output #defines for characteristcs of the MDM tree
#
sub output_mdmParams
{
    my $basedir = shift;
    my $fileRef;

    $fileRef = open_filehandle("$basedir/userspace/public/include/mdm_params.h");

    autogen_warning($fileRef);

    print $fileRef "#ifndef __MDM_PARAMS_H__\n";
    print $fileRef "#define __MDM_PARAMS_H__\n\n";

    doxygen_header($fileRef, "mdm_params.h");

    print $fileRef "/** maximum length of a param name, not including NULL */\n";
    print $fileRef "#define MAX_ACTUAL_MDM_PARAM_NAME_LENGTH $maxParamNameLength\n\n";

    print $fileRef "/** maximum actual instance depth */\n";
    print $fileRef "#define MAX_ACTUAL_MDM_INSTANCE_DEPTH    $maxInstanceDepth\n\n\n";

    print $fileRef "#endif /* __MDM_PARAMS_H__ */\n";

    close $fileRef;
}


#
# output the MDM tree as c data structures.
# We need to recursively go down to the bottom of our object node data
# structure and print from the bottom up.
#
sub output_mdm
{
    my $basedir = shift;
    my $fileRef;
    my $currProfile = "Baseline:1";


    output_profiled_validstrings($basedir);


    # The Baseline:1 profile gets some special header
    # The ordering of the includes is important, leaf profiles go first.
    $fileRef = get_profiled_filehandle($basedir, $currProfile);
    print $fileRef "#include \"cms.h\"\n";
    print $fileRef "#include \"mdm_types.h\"\n";
    print $fileRef "#include \"rcl.h\"\n";
    print $fileRef "#include \"stl.h\"\n";
    print $fileRef "#include \"validstrings.c\"\n";

    print $fileRef "#include \"QoSDynamicFlow_1.c\"\n";
    print $fileRef "#include \"QoS_1.c\"\n";

    print $fileRef "#include \"EthernetLAN_1.c\"\n";
    print $fileRef "#include \"USBLAN_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_WiFiLAN_1.c\"\n";
    print $fileRef "#include \"WiFiLAN_1.c\"\n";
    print $fileRef "#include \"IPPing_1.c\"\n";
    print $fileRef "#include \"Bridging_1.c\"\n";


    print $fileRef "#include \"X_BROADCOM_COM_AtmStats_1.c\"\n";
    print $fileRef "#include \"ATMLoopback_1.c\"\n";
    print $fileRef "#include \"DSLDiagnostics_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_ADSLWAN_1.c\"\n";
    print $fileRef "#include \"ADSLWAN_1.c\"\n";
    print $fileRef "#include \"EthernetWAN_1.c\"\n";
    print $fileRef "#include \"PTMWAN_1.c\"\n";
    print $fileRef "#include \"POTSWAN_1.c\"\n";

    print $fileRef "#include \"X_BROADCOM_COM_PSTNEndpoint_1.c\"\n";
    print $fileRef "#include \"TAEndpoint_1.c\"\n";
    print $fileRef "#include \"H323Endpoint_1.c\"\n";
    print $fileRef "#include \"MGCPEndpoint_1.c\"\n";
    print $fileRef "#include \"SIPEndpoint_1.c\"\n";
    print $fileRef "#include \"Endpoint_1.c\"\n";


    print $fileRef "#include \"Time_1.c\"\n";
    print $fileRef "#include \"DeviceAssociation_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_DigitalCertificates_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_IpSec_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_IpPrinting_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_DynamicDNS_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_AccessTimeRestriction_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_Security_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_SNMP_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_TR64_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_Debug_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_IPv6_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_Baseline_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_Diag_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_Upnp_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_DnsProxy_1.c\"\n";
    print $fileRef "#include \"X_BROADCOM_COM_P8021AG_1.c\"\n";
    print $fileRef "#include \"X_ITU_ORG_Gpon_1.c\"\n\n\n";
    print $fileRef "#include \"X_BROADCOM_COM_Gpon_1.c\"\n\n\n";

    # now go and output the tree
    output_nodes_recursively($basedir, $currProfile, $IGDRootObjRef);


    $fileRef = $profileToFileHandleHash{$currProfile};

    print $fileRef "MdmObjectNode igdRootObjNode = \n";
    $IGDRootObjRef->outputObjectNode($fileRef);
    print $fileRef ";\n\n";

    close_all_profiled_filehandles();
}


sub output_nodes_recursively
{
    my ($basedir, $currProfile, $objRef) = @_;
    my $tmpProfile;
    my $childObjectsArrayRef;
    my $numChildObjects;
    my $numSupportedChildObjects;
    my $childObjRef;
    my $objectsPrinted=0;
    my $fileRef;
    my $i=0;

#    print "Starting output_nodes_recursively\n";

    $childObjectsArrayRef = ${$objRef}{"ChildObjects"};
    $numChildObjects = @{$childObjectsArrayRef};
    $numSupportedChildObjects = $objRef->getSupportedChildObjectCount();

#    print "push a level: ${$objRef}{\"Name\"} children=$numChildObjects\n";

    #
    # If this object's profile is unspecifed, put it with the profile
    # of the last/parent object.
    #
    $tmpProfile = $objRef->getProfile;
    if ($tmpProfile ne "Unspecified")
    {
        $currProfile = $tmpProfile;
    }

    $fileRef = get_profiled_filehandle($basedir, $currProfile);

    if ($numChildObjects == 0)
    {
        # This node is at the bottom.
        # Dump out info about its parameters, and break out of the recursion.
        if ($objRef->isSupported())
        {
            $objRef->outputParams($fileRef);
        }
    }
    else
    {
        # This node has child nodes, recursively dump them out first
        # then dump the child nodes.

        my $lastChildProfile="noprofile";
        my $addEndif=0;
        my $profileDef;

        for ($i=0; $i < $numChildObjects; $i++)
        {
            $childObjRef = ${$childObjectsArrayRef}[$i];
            if (!defined($childObjRef))
            {
                die "could not find child at $i";
            }
            output_nodes_recursively($basedir, $currProfile, $childObjRef);
        }


        if ($objRef->isSupported())
        {
            # Now I can print out my own parameters
            $objRef->outputParams($fileRef);
        }

        if ($numSupportedChildObjects > 0)
        {
            # Finally, I can print the array of all children objects
            $objRef->outputChildObjectArrayHeader($fileRef);
        }

        for ($i=0; $i < $numChildObjects; $i++)
        {
            my $childProfile;

            $childObjRef = ${$childObjectsArrayRef}[$i];

            if ($childObjRef->isSupported() == 0)
            {
                # this object is not supported, skip it.
                next;
            }

            $objectsPrinted++;

            $childProfile = $childObjRef->getProfile();

            if (($childProfile ne $lastChildProfile) &&
                ($addEndif == 1))
            {
                print $fileRef "#endif /* $profileDef */\n";
                $addEndif = 0;
            }

            if (($childProfile ne $currProfile) &&
                ($childProfile ne "Unspecified") &&
                ($childProfile ne "$lastChildProfile"))
            {
                $lastChildProfile = $childProfile;
                $profileDef = Utils::convertProfileNameToPoundDefine($childProfile);
                print $fileRef "#ifdef $profileDef\n";
                $addEndif = 1;
            }

            $childObjRef->outputObjectNode($fileRef);
            if ($objectsPrinted < $numSupportedChildObjects)
            {
                print $fileRef ",\n";
            }
            else
            {
                print $fileRef "\n";
            }
        }

        if ($addEndif == 1)
        {
            print $fileRef "#endif /* $profileDef */\n";
        }

        if ($numSupportedChildObjects > 0)
        {
            print $fileRef "};\n\n";
        }
    }

# print "pop a level\n";

}


sub output_validstrings
{
    my $fileRef = $_[0];
    my $defStr;
    my $key;
    my $keyLen;
    my $offsetLen=20;

    foreach $key (keys (%allValidStrings))
    {
        $defStr = "MDMVS_" . convert_validstring($key);

        $keyLen = length($defStr);

        print $fileRef "#define $defStr";
        while ($keyLen < $offsetLen)
        {
            print $fileRef " ";
            $keyLen++;
        }
        print $fileRef " \"$key\"\n";
    }
}



sub output_valid_strings_hash
{
    my $fileRef = $_[0];
    my $profileName = $_[1];
    my $profileArrayRef;
    my $validValuesArrayRef;
    my ($i, $j) = (0, 0);

    print "dumping valid strings of $profileName\n";
    $profileArrayRef = $profileToValidStringsHash{$profileName};

    while (defined(${$profileArrayRef}[$i]))
    {
        $validValuesArrayRef = ${$profileArrayRef}[$i];

        $j = 0;
        while (defined(${$validValuesArrayRef}[$j]))
        {
            print $fileRef "[$i,$j]${$validValuesArrayRef}[$j]   ";
            $j++;
        }

        print $fileRef "\n";
        $i++;
    }
}


sub autogen_warning
{
    my $fileRef = shift;

    print $fileRef "/*\n";
    print $fileRef " * This file is automatically generated from the data-model spreadsheet.\n";
    print $fileRef " * Do not modify this file directly - You will lose all your changes the\n";
    print $fileRef " * next time this file is generated!\n";
    print $fileRef " */\n\n\n";
}


sub doxygen_header
{
    my $fileRef = shift;

    print $fileRef "/*!\\file $_[0] \n";
    print $fileRef " * \\brief Automatically generated header file $_[0]\n";
    print $fileRef " */\n\n\n";
}



###########################################################################
#
# Begin of main
#
###########################################################################

sub usage
{
    print "Usage: generate_from_dm.pl [command] [path_to_CommEngine_dir] < cms-data-model.xml\n";
    print "both arguments are mandatory.\n";
    print "command is: objectid, object, stringtable, mdm, prototypes, or skeletons\n";
}


if (!defined($ARGV[0]))
{
    usage();
    die "need first arg";
}

if (!defined($ARGV[1]))
{
    usage();
    die "need second arg";
}


if ($ARGV[0] eq "objectid")
{
    my $fileRef;
    my $build_dir = $ARGV[1];

    $fileRef = open_filehandle("$build_dir/userspace/public/include/mdm_objectid.h");

    print $fileRef "#ifndef __MDM_OBJECTID_H__\n";
    print $fileRef "#define __MDM_OBJECTID_H__\n\n\n";

    autogen_warning($fileRef);

    doxygen_header($fileRef, "mdm_objectid.h");

    output_mdmObjectIdFile($fileRef);

    print $fileRef "\n\n#endif /* __MDM_OBJECTID_H__ */\n";
}
elsif ($ARGV[0] eq "stringtable")
{
    my $fileRef;
    my $build_dir = $ARGV[1];

    $fileRef = open_filehandle("$build_dir/userspace/private/libs/cms_core/mdm_stringtable.c");

    autogen_warning($fileRef);

    output_mdmOidStringTable($fileRef);

    close $fileRef;
}
elsif ($ARGV[0] eq "handlerfuncstable")
{
    my $fileRef;
    my $build_dir = $ARGV[1];

    $fileRef = open_filehandle("$build_dir/userspace/private/libs/cms_core/mdm_handlerfuncstable.c");

    autogen_warning($fileRef);

    output_mdmOidHandlerFuncsTable($fileRef);

    close $fileRef;
}
elsif ($ARGV[0] eq "object")
{
    my $fileRef;
    my $build_dir = $ARGV[1];

    $fileRef = open_filehandle("$build_dir/userspace/public/include/mdm_object.h");

    print $fileRef "#ifndef __MDM_OBJECT_H__\n";
    print $fileRef "#define __MDM_OBJECT_H__\n\n\n";

    autogen_warning($fileRef);

    doxygen_header($fileRef, "mdm_object.h");

    print $fileRef "#include \"cms.h\"\n\n";

    output_mdmObjectFile($fileRef);

    print $fileRef "\n\n#endif /* __MDM_OBJECT_H__ */\n";
}
elsif ($ARGV[0] eq "mdm")
{
    my $build_dir = $ARGV[1];
    my $basedir;

    input_spreadsheet1();
    input_spreadsheet2();

    $basedir = $ARGV[1] . "/userspace/private/libs/mdm";

    output_mdm($basedir);

    output_mdmParams($ARGV[1]);
}
elsif ($ARGV[0] eq "validstrings")
{
    my $build_dir = $ARGV[1];
    my $fileRef;

    # we don't really need anything in spreadsheet1 for valid strings,
    # but we do need to chew up the input and get to spreadsheet2
    input_spreadsheet1();
    input_spreadsheet2();

    $fileRef = open_filehandle("$build_dir/userspace/public/include/mdm_validstrings.h");

    autogen_warning($fileRef);

    print $fileRef "#ifndef __MDM_VALIDSTRINGS_H__\n";
    print $fileRef "#define __MDM_VALIDSTRINGS_H__\n\n";

    output_validstrings($fileRef);

    print $fileRef "#endif /* __MDM_VALIDSTRINGS_H__ */\n";
}
elsif ($ARGV[0] eq "prototypes")
{
    my $build_dir = $ARGV[1];
    my ($fileRef1, $fileRef2);

    $fileRef1 = open_filehandle("$build_dir/userspace/private/libs/cms_core/rcl.h");
    $fileRef2 = open_filehandle("$build_dir/userspace/private/libs/cms_core/stl.h");

    autogen_warning($fileRef1);
    autogen_warning($fileRef2);

    print $fileRef1 "#ifndef __RCL_H__\n";
    print $fileRef2 "#ifndef __STL_H__\n";

    print $fileRef1 "#define __RCL_H__\n\n";
    print $fileRef2 "#define __STL_H__\n\n";

    print $fileRef1 "#include \"cms.h\"\n";
    print $fileRef1 "#include \"mdm.h\"\n";
    print $fileRef1 "#include \"cms_core.h\"\n\n";

    print $fileRef2 "#include \"cms.h\"\n";
    print $fileRef2 "#include \"mdm.h\"\n";
    print $fileRef2 "#include \"cms_core.h\"\n\n";

    output_prototypes($fileRef1, $fileRef2);

    print $fileRef1 "#endif /* __RCL_H__ */\n";
    print $fileRef2 "#endif /* __STL_H__ */\n";

    close $fileRef1;
    close $fileRef2;
}
elsif ($ARGV[0] eq "skeletons")
{
    my $build_dir = $ARGV[1];
    my ($fileRef1, $fileRef2);

    $fileRef1 = open_filehandle("$build_dir/userspace/private/libs/cms_core/linux/rcl_skel.c");
    $fileRef2 = open_filehandle("$build_dir/userspace/private/libs/cms_core/linux/stl_skel.c");

    print $fileRef1 "#include \"cms.h\"\n";
    print $fileRef1 "#include \"mdm.h\"\n";
    print $fileRef1 "#include \"cms_core.h\"\n\n";

    print $fileRef2 "#include \"cms.h\"\n";
    print $fileRef2 "#include \"mdm.h\"\n";
    print $fileRef2 "#include \"cms_core.h\"\n\n";

    output_skeletons($fileRef1, $fileRef2);

    close $fileRef1;
    close $fileRef2;
}

else
{
    usage();
    die "unrecognized command";
}

                   
