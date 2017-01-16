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
# This is a perl class.
# It stores data about a parameter in the MDM data model.
# It also outputs the the parameter in c language format so that
# it can be compiled into the cms_core library.
#
#

package GenParamNode;
require Exporter;
@ISA = qw(Exporter);


#
# Example of a class
# See Chapter 5, page 290 of Programming Perl
#


# @EXPORT exports the symbols by _default_ to the importing file
# They will not have to use the fully qualified name
@EXPORT = qw(
             $maxParamNameLength
             fillParamInfo
             isBool
             isSupported
             outputParamNode);


$maxParamNameLength = 0;
$_aaaaa = "blah";  #Is this a bug in Perl?  If the var is not here, module does not compile!

#
# constructor
#
sub new
{
    # {} returns an anonomus hash
    # bless causes the anonomous hash to be associated with this package.
    # return bless {};

    my $objref = {};
    bless $objref;

    return $objref;
}


#
# Fill in the information about this param node.
# This should have been passed in with the constructor, but I don't think
# the perl class constructor takes arguments.
#
sub fillParamInfo
{
    my $this = shift;
    my ($name, $type, $defaultValue, $profile, $denyActiveNotification, $forcedActiveNotification, $alwaysWriteToConfigFile, $neverWriteToConfigFile, $transferDataBuffer, $supportLevel) = @_;
    my $len;

    ${$this}{"name"} = $name;

    $len = length($name);
    if ($len > $maxParamNameLength)
    {
        $maxParamNameLength = $len;
    }
    
    ${$this}{"defaultValue"} = $this->fixupDefaultValue($defaultValue);

    ${$this}{"type"} = $this->fixupType($type);

    ${$this}{"profile"} = $profile;

    ${$this}{"denyActiveNotification"} = $denyActiveNotification;

    ${$this}{"forcedActiveNotification"} = $forcedActiveNotification;

    ${$this}{"alwaysWriteToConfigFile"} = $alwaysWriteToConfigFile;

    ${$this}{"neverWriteToConfigFile"} = $neverWriteToConfigFile;

    ${$this}{"transferDataBuffer"} = $transferDataBuffer;

    ${$this}{"supportLevel"} = $supportLevel;
}


sub setMinMaxValues
{
    my $this = shift;
    my ($min, $max) = @_;

    if (${$this}{"type"} eq "MPT_BOOLEAN") {
        die "setMinMaxValues called on boolean param!\n";
    }
    elsif (${$this}{"type"} eq "MPT_STRING") {
        die "setMinMaxValues called on string param!\n";
    }
    elsif (${$this}{"type"} eq "MPT_BASE64") {
        die "setMinMaxValues called on base64 param!\n";
    }

    ${$this}{"minValue"} = $min;
    ${$this}{"maxValue"} = $max;
}

sub setMaxLength
{
    my $this = shift;
    my ($maxLength) = @_;

    if ((${$this}{"type"} ne "MPT_STRING") && 
        (${$this}{"type"} ne "MPT_BASE64")) { 
        die "setMaxLength called on non-string param!\n";
    }

    ${$this}{"maxLength"} = $maxLength;
}

sub setValidValuesArray
{
    my $this = shift;
    my ($vsa) = @_;

    if (${$this}{"type"} ne "MPT_STRING") {
        die "setValidValuesArray called on non-string param!\n";
    }

    ${$this}{"validValuesArray"} = $vsa;
#    print "setValidValuesArray: ${$this}{name} = ${$this}{validValuesArray}\n";
}


#
# Return 1 if this param node is supported either as ReadOnly or ReadWrite
#
sub isSupported
{
    my $this = shift;

    if (${$this}{"supportLevel"} eq "NotSupported")
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

#
# Return 1 if this param node is of BOOL type
#
sub isBool
{
    my $this = shift;

    if (${$this}{"type"} eq "MPT_BOOLEAN")
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

#
# Return 1 if this param node is of string type
#
sub isString
{
    my $this = shift;

    if (${$this}{"type"} eq "MPT_STRING")
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

#
# Return 1 if this param node is of int type
#
sub isInt
{
    my $this = shift;

    if (${$this}{"type"} eq "MPT_INTEGER")
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

#
# Return 1 if this param node is of uint type
#
sub isUnsignedInt
{
    my $this = shift;

    if (${$this}{"type"} eq "MPT_UNSIGNED_INTEGER")
    {
        return 1;
    }
    else
    {
        return 0;
    }
}



#
# Print out the MdmParamNode structure for this parameter.
# offset in object is now calculated at runtime so that parameter
# profile defines can be accomodated.  See oal_mdm.c.
#
sub outputParamNode
{
    my ($this, $fh, $parentProfile) = @_;
    my $nodeFlags;
    my $def;
    my $printEndif = 0;
    my ($vMinPrefix, $vMaxPrefix);

    $nodeFlags = $this->getNodeFlags();

    if (($parentProfile ne ${$this}{"profile"}) && !(${$this}{"profile"} =~ /unspecified/i))
    {
        $def = Utils::convertProfileNameToPoundDefine(${$this}{"profile"});
        print $fh "#ifdef $def\n";
        $printEndif = 1;
    }

    print $fh "   { \"${$this}{\"name\"}\",\n";
    print $fh "     NULL, /* parent ptr */\n";
    print $fh "     ${$this}{\"type\"},\n";
    print $fh "     $nodeFlags, 0, /* node flags, offset in obj */\n";
    if (${$this}{"defaultValue"} eq "NULL")
    {
        print $fh "     NULL, /* default val */\n";
    }
    else
    {
        print $fh "     \"${$this}{\"defaultValue\"}\", /* default val */\n";
    }
    print $fh "     NULL, /* suggested val */\n";


    if (defined(${$this}{"minValue"}))
    {
        # this must be either an int or uint
        if (${$this}{"maxValue"} eq "4294967295")
        {
            ${$this}{"maxValue"} .= "UL";
        }

        print $fh "     {(void *) ${$this}{minValue}, (void *) ${$this}{maxValue}}  /* validator data */\n";  
    }
    elsif (defined(${$this}{"maxLength"}))
    {
        print $fh "     {(void *) 0, (void *) ${$this}{maxLength}}  /* validator data */\n";  
    }
    elsif (defined(${$this}{"validValuesArray"}))
    {
        print $fh "     { ${$this}{validValuesArray}, (void *) 0}  /* validator data */\n";  
    }
    else
    {
        print $fh "     {(void *) 0, (void *) 0}  /* validator data */\n";  
    }

    print $fh "   },\n";

    if ($printEndif == 1)
    {
        print $fh "#endif\n";
    }
}



############################################################################
#
# These are private functions.
#
############################################################################


#
# Take a default/initial/suggested value string and transform it to
# a form that is usable in the c tree files.
#
sub fixupDefaultValue
{
    my ($this, $name) = @_;

#    chomp($name);

    if (($name eq "-") || ($name eq "NA"))
    {
        return "NULL";
    }

    # this matches quoted strings
    if ($name =~ /&quot;([\w-]+)&quot;/)
    {
        return $1;
    }

    # this matches strings in <brackets>
    if ($name =~ /&lt;([\w-]+)&gt;/)
    {
        return $1;
    }

    # strip out trailing space on numbers
    if ($name =~ /^([\-]*[\d]+)[\s]*$/)
    {
        return $1;
    }

    return $name;
}


#
# Take the type string from the MDM spreadsheet and transform it
# to a MdmParamTypes enumeration.
#
sub fixupType
{
    my ($this, $type) = @_;

    if ($type eq "string")
    {
        return "MPT_STRING";
    }
    elsif ($type eq "int")
    {
        return "MPT_INTEGER";
    }
    elsif ($type eq "unsignedInt")
    {
        return "MPT_UNSIGNED_INTEGER";
    }
    elsif ($type eq "boolean")
    {
        return "MPT_BOOLEAN";
    }
    elsif ($type eq "dateTime")
    {
        return "MPT_DATE_TIME";
    }
    elsif ($type eq "base64")
    {
        return "MPT_BASE64";
    }
    else
    {
        die "Unrecognized param type $type on ${$this}{\"name\"}";
    }
}

#
# Return a string which is the node flags for this ParamNode
#
sub getNodeFlags
{
    my $this = shift;
    my $flagString = "0";

    if (defined(${$this}{"denyActiveNotification"}))
    {
        if ((${$this}{"denyActiveNotification"} =~ /true/i) ||
            (${$this}{"denyActiveNotification"} =~ /yes/i))
        {
            $flagString = "PRN_DENY_ACTIVE_NOTIFICATION";
        }
    }

    if (defined(${$this}{"forcedActiveNotification"}))
    {
        if ((${$this}{"forcedActiveNotification"} =~ /true/i) ||
            (${$this}{"forcedActiveNotification"} =~ /yes/i))
        {
            $flagString = "PRN_FORCED_ACTIVE_NOTIFICATION";
        }
    }

    if (defined(${$this}{"alwaysWriteToConfigFile"}))
    {
        if ((${$this}{"alwaysWriteToConfigFile"} =~ /true/i) ||
            (${$this}{"alwaysWriteToConfigFile"} =~ /yes/i))
        {
            if ($flagString eq "0")
            {
               $flagString = "PRN_ALWAYS_WRITE_TO_CONFIG_FILE";
            }
            else
            {
               $flagString = $flagString . " | PRN_ALWAYS_WRITE_TO_CONFIG_FILE";
            }
        }
    }

    if (defined(${$this}{"neverWriteToConfigFile"}))
    {
        if ((${$this}{"neverWriteToConfigFile"} =~ /true/i) ||
            (${$this}{"neverWriteToConfigFile"} =~ /yes/i))
        {
            if ($flagString eq "0")
            {
               $flagString = "PRN_NEVER_WRITE_TO_CONFIG_FILE";
            }
            else
            {
               $flagString = $flagString . " | PRN_NEVER_WRITE_TO_CONFIG_FILE";
            }
        }
    }

    if (defined(${$this}{"transferDataBuffer"}))
    {
        if ((${$this}{"transferDataBuffer"} =~ /true/i) ||
            (${$this}{"transferDataBuffer"} =~ /yes/i))
        {
            if ($flagString eq "0")
            {
               $flagString = "PRN_TRANSFER_DATA_BUFFER";
            }
            else
            {
               $flagString = $flagString . " | PRN_TRANSFER_DATA_BUFFER";
            }
        }
    }

    if (${$this}{"supportLevel"} =~ /ReadWrite/i)
    {
        if ($flagString eq "0")
        {
            $flagString = PRN_WRITABLE;
        }
        else
        {
            $flagString = $flagString . " | PRN_WRITABLE";
        }
    }

    return $flagString;
}
