/* 
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000-2004 Silicon Graphics, Inc. All rights reserved.
 */


#ifndef _ASM_IA64_SN_SN_CPUID_H
#define _ASM_IA64_SN_SN_CPUID_H

#include <linux/smp.h>
#include <asm/sn/addrs.h>
#include <asm/sn/pda.h>
#include <asm/intrinsics.h>


/*
 * Functions for converting between cpuids, nodeids and NASIDs.
 * 
 * These are for SGI platforms only.
 *
 */




/*
 *  Definitions of terms (these definitions are for IA64 ONLY. Other architectures
 *  use cpuid/cpunum quite defferently):
 *
 *	   CPUID - a number in range of 0..NR_CPUS-1 that uniquely identifies
 *		the cpu. The value cpuid has no significance on IA64 other than
 *		the boot cpu is 0.
 *			smp_processor_id() returns the cpuid of the current cpu.
 *
 * 	   CPU_PHYSICAL_ID (also known as HARD_PROCESSOR_ID)
 *		This is the same as 31:24 of the processor LID register
 *			hard_smp_processor_id()- cpu_physical_id of current processor
 *			cpu_physical_id(cpuid) - convert a <cpuid> to a <physical_cpuid>
 *			cpu_logical_id(phy_id) - convert a <physical_cpuid> to a <cpuid> 
 *				* not real efficient - don't use in perf critical code
 *
 *         SLICE - a number in the range of 0 - 3 (typically) that represents the
 *		cpu number on a brick.
 *
 *	   SUBNODE - (almost obsolete) the number of the FSB that a cpu is
 *		connected to. This is also the same as the PI number. Usually 0 or 1.
 *
 *	NOTE!!!: the value of the bits in the cpu physical id (SAPICid or LID) of a cpu has no 
 *	significance. The SAPIC id (LID) is a 16-bit cookie that has meaning only to the PROM.
 *
 *
 * The macros convert between cpu physical ids & slice/nasid/cnodeid.
 * These terms are described below:
 *
 *
 * Brick
 *          -----   -----           -----   -----       CPU
 *          | 0 |   | 1 |           | 0 |   | 1 |       SLICE
 *          -----   -----           -----   -----
 *            |       |               |       |
 *            |       |               |       |
 *          0 |       | 2           0 |       | 2       FSB SLOT
 *             -------                 -------  
 *                |                       |
 *                |                       |
 *                |                       |
 *             ------------      -------------
 *             |          |      |           |
 *             |    SHUB  |      |   SHUB    |        NASID   (0..MAX_NASIDS)
 *             |          |----- |           |        CNODEID (0..num_compact_nodes-1)
 *             |          |      |           |
 *             |          |      |           |
 *             ------------      -------------
 *                   |                 |
 *                           
 *
 */

#ifndef CONFIG_SMP
#define cpu_physical_id(cpuid)			((ia64_getreg(_IA64_REG_CR_LID) >> 16) & 0xffff)
#endif


#define get_node_number(addr)			NASID_GET(addr)

/*
 * NOTE: on non-MP systems, only cpuid 0 exists
 */

extern short physical_node_map[];			/* indexed by nasid to get cnode */

/*
 * Macros for retrieving info about current cpu
 */
#define get_nasid()			(nodepda->phys_cpuid[smp_processor_id()].nasid)
#define get_subnode()			(nodepda->phys_cpuid[smp_processor_id()].subnode)
#define get_slice()			(nodepda->phys_cpuid[smp_processor_id()].slice)
#define get_cnode()			(nodepda->phys_cpuid[smp_processor_id()].cnode)
#define get_sapicid()			((ia64_getreg(_IA64_REG_CR_LID) >> 16) & 0xffff)

/*
 * Macros for retrieving info about an arbitrary cpu
 *	cpuid - logical cpu id
 */
#define cpuid_to_nasid(cpuid)		(nodepda->phys_cpuid[cpuid].nasid)
#define cpuid_to_subnode(cpuid)		(nodepda->phys_cpuid[cpuid].subnode)
#define cpuid_to_slice(cpuid)		(nodepda->phys_cpuid[cpuid].slice)
#define cpuid_to_cnodeid(cpuid)		(physical_node_map[cpuid_to_nasid(cpuid)])


/*
 * Dont use the following in performance critical code. They require scans
 * of potentially large tables.
 */
extern int nasid_slice_to_cpuid(int, int);
#define nasid_slice_to_cpu_physical_id(nasid, slice)			\
	cpu_physical_id(nasid_slice_to_cpuid(nasid, slice))

/*
 * cnodeid_to_nasid - convert a cnodeid to a NASID
 *	Macro relies on pg_data for a node being on the node itself.
 *	Just extract the NASID from the pointer.
 *
 */
#define cnodeid_to_nasid(cnodeid)	pda->cnodeid_to_nasid_table[cnodeid]
 
/*
 * nasid_to_cnodeid - convert a NASID to a cnodeid
 */
#define nasid_to_cnodeid(nasid)		(physical_node_map[nasid])

/*
 * partition_coherence_id - cget the coherence ID of the current partition
 */
#define partition_coherence_id()	(get_nasid() >> 9)

#endif /* _ASM_IA64_SN_SN_CPUID_H */

