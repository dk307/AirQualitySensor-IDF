
const path = require('path');
const gulp = require('gulp');
const through = require('through2');
const htmlmin = require('html-minifier');
const gzip = require('gulp-gzip');
const concat = require('gulp-concat');
const imagemin = require('gulp-imagemin');
const htmlvalidate = require('gulp-html');
var browserSync = require('browser-sync').create();
var watch = require('gulp-watch');
var reload = browserSync.reload;
var stripcomments = require('gulp-strip-comments');
var htmlminify = require('gulp-html-minifier-terser');
var minify = require('gulp-minify');



var log = require('fancy-log');

const baseFolder = 'main/web/static/';
const dataFolder = 'main/web/data/';
const staticFolder = 'main/web/include/';
const staticSDcardFolder = 'main/sdcard/web/';

var toHeader = function (name, debug) {
    return through.obj(function (source, encoding, callback) {
        var parts = source.path.split(path.sep);
        var filename = parts[parts.length - 1];
        var safename = name || filename.split('.').join('_');

        // Generate output
        var output = '';
        output += '#define ' + safename + '_len ' + source.contents.length + '\n';
        output += 'const uint8_t ' + safename + '[] = {';
        for (var i = 0; i < source.contents.length; i++) {
            if (i > 0) { output += ','; }
            if (0 === (i % 20)) { output += '\n'; }
            output += '0x' + ('00' + source.contents[i].toString(16)).slice(-2);
        }
        output += '\n};';

        // clone the contents
        var destination = source.clone();
        destination.path = source.path + '.h';
        destination.contents = Buffer.from(output);

        if (debug) {
            console.info('Image ' + filename + ' \tsize: ' + source.contents.length + ' bytes');
        }

        callback(null, destination);
    });

};

gulp.task('html', function () {
    return gulp.src(baseFolder + '*.html').
        pipe(htmlvalidate()).
        pipe(htmlminify({
            collapseWhitespace: true,
            minifyJS: true,
            minifyCSS: true,
            removeComments: true,
            removeAttributeQuotes: true,
            collapseBooleanAttributes: true,
            collapseInlineTagWhitespace: true,
            decodeEntities: true,
            minifyURLs: true,
            removeEmptyAttributes: true,
            removeRedundantAttributes: true,
            sortAttributes: true,
            sortClassName: true
        })).
        pipe(htmlvalidate()). // validate again in end
        pipe(gzip({ gzipOptions: { level: 9 } })).
        pipe(gulp.dest(dataFolder)).
        pipe(toHeader(null, true)).
        pipe(gulp.dest(staticFolder));
});

gulp.task('js', function () {
    return gulp.src(baseFolder + '/js/*.js').
        pipe(stripcomments()).
        pipe(concat("s.js")).
        pipe(minify()).
        pipe(gzip({ gzipOptions: { level: 9 } })).
        pipe(gulp.dest(dataFolder)).
        pipe(gulp.dest(staticSDcardFolder));
});

gulp.task('js-extra', function () {
    return gulp.src(baseFolder + '/js/extra/*.js').
        pipe(stripcomments()).
        pipe(minify()).
        pipe(gzip({ gzipOptions: { level: 9 } })).
        pipe(gulp.dest(dataFolder)).
        pipe(gulp.dest(staticSDcardFolder));
});

gulp.task('css', function () {
    return gulp.src(baseFolder + '/css/*.css').
        pipe(minify()).
        pipe(gzip({ gzipOptions: { level: 9 } })).
        pipe(gulp.dest(dataFolder)).
        pipe(gulp.dest(staticSDcardFolder));
});

gulp.task('images', function () {
    return gulp.src(baseFolder + '/media/*.png').
        pipe(imagemin()).
        pipe(gulp.dest(dataFolder)).
        pipe(gulp.dest(staticSDcardFolder));
});

gulp.task('default', gulp.series('html', 'js', 'js-extra', 'css', 'images'));


gulp.task('serve', function () {
    browserSync.init({
        server: {
            baseDir: baseFolder
        }
    });
});

gulp.task('watchHtml', function () { gulp.watch(baseFolder + '**/*.html', reload) });
gulp.task('watchJs', function () { gulp.watch(baseFolder + '**/*.js', reload) });
gulp.task('watchCss', function () { gulp.watch(baseFolder + '**/*.css', reload) });
gulp.task('watchPng', function () { gulp.watch(baseFolder + '**/*.png', reload) });

// Static server
gulp.task('webserver', gulp.parallel(['serve', 'watchHtml', 'watchJs', 'watchCss', 'watchPng']));
