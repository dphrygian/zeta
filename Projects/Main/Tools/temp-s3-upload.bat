@echo off

REM: Later, replace this with an uploader to private queue (or do it by hand to be sure I never put earlier stuff in the queue)

REM Upload first map to 02.cpk as if it were an older entry from the queue
aws s3 cp ../BakedContent/Cloud/01-01-1.cpk s3://zeta-public/02.cpk --acl public-read
aws s3 cp ../BakedContent/Cloud/01-01-2.cpk s3://zeta-public/01.cpk --acl public-read
