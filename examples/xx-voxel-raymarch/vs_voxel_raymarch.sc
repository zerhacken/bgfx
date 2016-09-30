$input a_position
$output v_texcoord0

/*
 * Copyright 2016 Rasmus Christian Pedersen. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "../common/common.sh"

void main()
{
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
	v_texcoord0 = a_position;
}