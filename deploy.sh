hexo generate
cp -R public/* .deploy/agehua.github.io
cd .deploy/agehua.github.io
git add .
git commit -m "update"
git push origin master
