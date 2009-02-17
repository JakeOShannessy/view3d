#!/bin/bash
RSYNC_RSH=ssh rsync --delete --progress -Cvaz /home/john/view3d-website/ jdpipe,view3d@web.sourceforge.net:/home/groups/v/vi/view3d/htdocs/

