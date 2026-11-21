/*
 * Copyright 2025, California Institute of Technology.
 * All rights reserved. U.S. Government sponsorship acknowledged.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
// nisar_version.h

#ifndef NISAR_VERSION_H
#define NISAR_VERSION_H

// Single source of the driver version: reported as DRIVER_VERSION by both the
// in-tree GDAL build and the standalone plugin build (whose CMakeLists.txt
// parses this file for project(VERSION ...)).
#define NISAR_DRIVER_VERSION "0.7.3"

#endif  // NISAR_VERSION_H
