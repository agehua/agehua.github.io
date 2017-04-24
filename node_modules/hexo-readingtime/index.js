'use strict';
/* globals hexo: true */
var readingTime = require('reading-time');

hexo.extend.filter.register('before_post_render', function(data) {
	data.readingTime = readingTime(data.content);
	return data;
});