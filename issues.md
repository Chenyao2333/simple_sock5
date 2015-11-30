There are some strange bugs that i can't find any clue of it.

1. Connection be closed before completely receive data. Especially it often occurs when resource type is image. Last times happened on https://assets-cdn.github.com/images/modules/home/desktop-mac.png?v=1 （... , i can't replicate the problem.)

2. When the connect receive a "Connection reset by peer" error, the remote side would be exited without any other information(thanks for GFW help me found this problem). （Looks like it has been fixed, just check connection befor sending)
