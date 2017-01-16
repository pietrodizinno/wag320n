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
# It stores data about an object in the MDM data model.
# It also outputs the object in c language format so that
# it can be compiled into the cms_core library.
#
#

package GenObjectNode;
require Exporter;
@ISA = qw(Exporter);


#
# Example of a class
# See Chapter 5, page 290 of Programming Perl
#


# @EXPORT exports the symbols by _default_ to the importing file
# They will not have to use the fully qualified name
@EXPORT = qw(
             $maxInstanceDepth
             fillObjectInfo
             addChildObject
             addParamNode
             getSupportedChildObjectCount
             getParamCount
             getProfile
             outputChildObjectArrayHeader
             outputObjectNode);

$maxInstanceDepth=0;
$_bbbbb = "blah";  #Is this a bug in Perl?  If the var is not here, module does not compile!

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


    ${$objref}{"ChildObjects"} = [];
    ${$objref}{"Params"} = [];

    return $objref;
}

#
# Fill in the information about this object node.
# This should have been passed in with the constructor, but I don't think
# the perl class constructor takes arguments.
#
sub fillObjectInfo
{
    my $this = shift;
    my ($oid, $depth, $name, $profile, $rw, $abbrev, $prune) = @_;

    ${$this}{"oid"} = $oid;
    ${$this}{"depth"} = $depth;
    ${$this}{"name"} = $name;
    ${$this}{"profile"} = $profile;
    ${$this}{"supportLevel"} = $rw;
    ${$this}{"shortObjectName"} = $abbrev;
    ${$this}{"pruneWriteToConfigFile"} = $prune;

#    print "$name->${$this}{\"profile\"}\n";
}


#
# Add a child object onto my array
#
sub addChildObject
{
    my $this = shift;
    my ($objid, $name, $childRef) = @_;
    my $childObjectsArrayRef;

    
#    print ("ObjectNode: adding (${$this}{\"Depth\"})${$this}{\"Name\"} -> (${$childRef}{\"Depth\"})${$childRef}{\"Name\"}\n\n");

    # Append this child to the array of child objects
    $childObjectsArrayRef = ${$this}{"ChildObjects"};
    @{$childObjectsArrayRef} = (@{$childObjectsArrayRef}, $childRef);

#    print "ObjectNode: $keystring = $name\n";
}


#
# Add a parameter node which belongs to this object.
#
sub addParamNode
{
    my $this = shift;
    my ($paramRef) = @_;
    my $paramArrayRef;

    
#    print ("ObjectNode: adding ${$this}{\"Name\"} -> ${$paramRef}{\"Name\"}\n\n");

    $paramArrayRef = ${$this}{"Params"};
    @{$paramArrayRef} = (@{$paramArrayRef}, $paramRef);
}


#
# Return the number of supported child objects
#
sub getSupportedChildObjectCount
{
    my $this = shift;
    my $childObjectArrayRef;
    my $childObjectRef;
    my $num;
    my $numSupported = 0;
    my $i;

    $childObjectArrayRef = ${$this}{"ChildObjects"};
    $num = @{$childObjectArrayRef};

    for ($i=0; $i < $num; $i++)
    {
        $childObjectRef = ${$childObjectArrayRef}[$i];
        if ($childObjectRef->isSupported())
        {
            $numSupported++;
        }
    }

    return $numSupported;
}


#
# Return the number of supported parameters belonging to this object
#
sub getSupportedParamCount
{
    my $this = shift;
    my $paramArrayRef;
    my ($num, $numSupported, $i);

    $paramArrayRef = ${$this}{"Params"};
    $num = @{$paramArrayRef};
    $numSupported = 0;

    for ($i=0; $i < $num; $i++)
    {
        $paramRef = ${$paramArrayRef}[$i];
        if ($paramRef->isSupported())
        {
            $numSupported++;
        }
    }

    return $numSupported;
}


#
# Return the profile name of this object
#
sub getProfile
{
    my $this = shift;

    return ${$this}{"profile"};
}


#
# Print out the header to the child object array
#
sub outputChildObjectArrayHeader
{
    my ($this, $fhRef) = @_;
    my $arrayName;

    print $fhRef "\n";
    print $fhRef "/* child objects of ${$this}{\"name\"} */\n";
    print $fhRef "/* in profile ${$this}{\"profile\"} */\n";

    print $fhRef "MdmObjectNode ";
    $arrayName = $this->getChildObjArrayName() . "[]";
    print $fhRef "$arrayName = { \n";
}


#
# Print out the MdmObjectNode structure for this object.
#
sub outputObjectNode
{
    my ($this, $fh) = @_;
    my ($numName, $numChildObjectNodes, $numParamNodes);
    my ($childObjArrayName, $paramArrayName, $childObjCount, $paramCount);
    my $instanceDepth;
    my $objName;


    $numSupportedChildObjectNodes = $this->getSupportedChildObjectCount();
    if ($numSupportedChildObjectNodes == 0)
    {
        $numName = 0;
        $childObjArrayName = "NULL";
        $childObjCount = "0";
    }
    else
    {
        $childObjArrayName = $this->getChildObjArrayName();
        $childObjCount = "sizeof($childObjArrayName)/sizeof(MdmObjectNode)";
    }

    $numParamNodes = $this->getSupportedParamCount();
    if ($numParamNodes == 0)
    {
        $paramArrayName = "NULL";
        $paramCount = 0;
    }
    else
    {
        $paramArrayName = $this->getParamArrayName();
        $paramCount = "sizeof($paramArrayName)/sizeof(MdmParamNode)";
    }


    $instanceDepth = $this->getInstanceDepth();
    if ($instanceDepth > $maxInstanceDepth)
    {
        $maxInstanceDepth = $instanceDepth;
    }
    
    $objName = $this->getLastNameComponent();
    $nodeFlags = $this->getNodeFlags();

    print $fh "   { ${$this}{\"oid\"}, /* ${$this}{\"name\"} */\n";
    print $fh "     \"$objName\", /* object name */\n";
    print $fh "     $nodeFlags, $instanceDepth,/* node flags, instance depth */\n";
    print $fh "     NULL,        /* parent objNode ptr */\n";
    print $fh "     {0, DEFAULT_ACCESS_BIT_MASK, 0, DEFAULT_NOTIFICATION_VALUE, 0, 0}, /* nodeattr value of this objNode */\n";
    print $fh "     NULL, /* pointer to instances of nodeattr values for parameter nodes */\n";
    print $fh "     $paramCount, /* num param nodes */\n";
    print $fh "     $childObjCount, /* num obj nodes */\n";
    print $fh "     $paramArrayName, /* param nodes array */\n";
    print $fh "     $childObjArrayName, /* child obj array */\n";
    print $fh "     NULL  /* obj data */\n";  
    print $fh "   }";

}


#
# Print out the parameter nodes for this object.
#
sub outputParams
{
    my ($this, $fh) = @_;
    my ($numParams, $numSupportedParams, $i, $paramArrayRef);
    my $arrayName;
    my $paramsPrinted = 0;

    $paramArrayRef = ${$this}{"Params"};
    $numParams = @{$paramArrayRef};
    $numSupportedParams = $this->getSupportedParamCount();

    if ($numParams == 0)
    {
        return;
    }

    $arrayName = $this->getParamArrayName();
    $arrayName = $arrayName . "[]";

    print $fh "/* params for ${$this}{\"name\"} */\n";
    print $fh "/* in profile ${$this}{\"profile\"} */\n";
    print $fh "MdmParamNode $arrayName = {\n";

    for ($i=0; $i < $numParams; $i++)
    {
        $paramRef = ${$paramArrayRef}[$i];

        if ($paramRef->isSupported() == 0)
        {
            # parameter is not supported, skip it.
            next;
        }

        $paramsPrinted++;

        $paramRef->outputParamNode($fh, ${$this}{"profile"});
    }

    print $fh "};\n\n";

}



############################################################################
#
# These are private functions.
#
############################################################################


#
# Return the childObjArray name from the abbreviated name
#
sub getChildObjArrayName
{
    my $this = shift;
    my $name;

    $name = $this->getArrayName();

    $name = $name . "ChildObjArray";

    return $name;
}

#
# Return the paramArray name from the abbreviated name
#
sub getParamArrayName
{
    my $this = shift;
    my $name;

    $name = $this->getArrayName();

    $name = $name . "ParamArray";

    return $name;
}

#
# Return the transformed abbreviated object name suitable for use
# in the array names
#
sub getArrayName
{
    my $this = shift;
    my $arrayName;
    my ($firstLetter, $restOfWord);

    # Get rid of Object from the abbreviated name and lower case the first letter.
    # Objects starting with IGD are special case, igd becomes lower cased
    if (${$this}{"shortObjectName"} =~ /^IGD([\w]*)Object/)
    {
        $firstLetter = "igd";
        if (defined($1))
        {
            $restOfWord = $1;
        }
        else
        {
            $restOfWord = "";
        }
    }
    else
    {
        ${$this}{"shortObjectName"} =~ /([\w])([\w]+)Object/;

        if (!defined($1) || !defined($2))
        {
            die "Could not extract ${$this}{\"shortObjectName\"}";
        }

        $firstLetter = $1;
        $restOfWord = $2;

        $firstLetter =~ tr/[A-Z]/[a-z]/;
    }
    
    $arrayName = $firstLetter . $restOfWord;

    return $arrayName;
}


#
# Return the number of instance markers (".{i}.") in the object name
#
sub getInstanceDepth
{
    my $this = shift;
    my $name;
    my $depth = 0;

    $name = ${$this}{"name"};

    while ((defined($name)) && ($name =~ /([\w\.]+.{i}.)([\w\.{}]*)/))
    {
        $depth += 1;
        $name = $2;
    }

    return $depth;
}


#
# Return the last name component from the generic object name.
#
sub getLastNameComponent
{
    my $this = shift;
    my $name;
    my $lastComp;

    $name = ${$this}{"name"};

    if ($name eq "InternetGatewayDevice.")
    {
        $lastComp = "InternetGatewayDevice";
    }
    elsif ($name =~ /[\w\.{}]+\.([\w]+)\.{i}\.$/)
    {
        # name ends in .{i}.
        $lastComp = $1;
    }
    elsif ($name =~ /[\w\.{}]+\.([\w]+)\.$/)
    {
        # name ends in with some letters and then . (not .{i}.)
        $lastComp = $1;
    }


    if (!defined($lastComp))
    {
        die "Could not find last component of $name";
    }

    # print "lastComp=$lastComp\n";

    return $lastComp;
}


#
# Return a string which is the node flags for the ObjectNode
#
sub getNodeFlags
{
    my $this = shift;
    my $flagString = "0";

    #
    # First check if this node is an instance node. 
    # An instance node has a name that ends with .{i}.
    #
    if (${$this}{"name"} =~ /\.{i}\.$/)
    {
        $flagString = OBN_INSTANCE_NODE;

        #
        # If this node is an instance node, see if we support dynamic
        # add/delete of instances.
        #
        if (${$this}{"supportLevel"} eq "DynamicInstances")
        {
            $flagString = $flagString . " | OBN_DYNAMIC_INSTANCES";
        }
    }

    if ((${$this}{"pruneWriteToConfigFile"} =~ /true/i) ||
        (${$this}{"pruneWriteToConfigFile"} =~ /yes/i))
    {
       if ($flagString eq "0")
       {
          $flagString = "OBN_PRUNE_WRITE_TO_CONFIG_FILE";
       }
       else
       {
           $flagString = $flagString . " | OBN_PRUNE_WRITE_TO_CONFIG_FILE";
       }
    }

    return $flagString;
}


#
# Return 1 if this object node is supported as Present, DynamicInstances, or MultipleInstances
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
