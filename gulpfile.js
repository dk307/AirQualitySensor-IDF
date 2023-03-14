
const path = require('path');
const gulp = require('gulp');
const through = require('through2');
const gzip = require('gulp-gzip');
const concat = require('gulp-concat');
const imagemin = require('gulp-imagemin');
const htmlvalidate = require('gulp-html');
var browserSync = require('browser-sync').create();
var reload = browserSync.reload;
var stripcomments = require('gulp-strip-comments');
var htmlminify = require('gulp-html-minifier-terser');
var minify = require('gulp-minify');
var shajs = require('sha.js');
var log = require('fancy-log');

const baseFolder = 'static/';
const tempWebFolder = baseFolder + 'web/temp/data/';
const displayWebFolder = baseFolder + 'display/temp/data/';
const staticWebInclude = 'main/generated/web/include';
const staticDisplayInclude = 'main/generated/display/include';
const sdcardFolder = 'sdcard/';

var toHeader = function(name, debug) {
  return through.obj(function(source, encoding, callback) {
    var parts = source.path.split(path.sep);
    var filename = parts[parts.length - 1];
    var safename = name || filename.split('.').join('_');

    var sha = shajs('sha256').update(source.contents).digest('hex');
    // console.info(sha);

    // Generate output
    var output = '';
    output += '#define ' + safename + '_len ' + source.contents.length + '\n';
    output += 'const char ' + safename + '_sha256[] = "' + sha + '";\n';
    output += 'const uint8_t ' + safename + '[] = {';
    for (var i = 0; i < source.contents.length; i++) {
      if (i > 0) {
        output += ',';
      }
      if (0 === (i % 20)) {
        output += '\n';
      }
      output += '0x' + ('00' + source.contents[i].toString(16)).slice(-2);
    }
    output += '\n};';

    // clone the contents
    var destination = source.clone();
    destination.path = source.path + '.h';
    destination.contents = Buffer.from(output);

    if (debug) {
      console.info(
          'Image ' + filename + ' \tsize: ' + source.contents.length +
          ' bytes');
    }

    callback(null, destination);
  });
};

gulp.task('html', function() {
  return gulp.src(baseFolder + 'web/*.html')
      .pipe(htmlvalidate())
      .pipe(htmlminify({
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
      }))
      .pipe(htmlvalidate())
      .pipe(gzip({gzipOptions: {level: 9}}))
      .pipe(gulp.dest(tempWebFolder))
      .pipe(toHeader(null, true))
      .pipe(gulp.dest(staticWebInclude));
});

gulp.task('js', function() {
  return gulp.src(baseFolder + 'web/js/*.js')
      .pipe(stripcomments())
      .pipe(concat('s.js'))
      .pipe(gzip({gzipOptions: {level: 9}}))
      .pipe(gulp.dest(tempWebFolder))
      .pipe(gulp.dest(sdcardFolder + '/web'));
});

gulp.task('js-extra', function() {
  return gulp.src(baseFolder + 'web/js/extra/*.js')
      .pipe(stripcomments())
      .pipe(gzip({gzipOptions: {level: 9}}))
      .pipe(gulp.dest(tempWebFolder))
      .pipe(gulp.dest(sdcardFolder + '/web/extra'));
});

gulp.task('css', function() {
  return gulp.src(baseFolder + 'web/css/*.css')
      .pipe(minify())
      .pipe(gzip({gzipOptions: {level: 9}}))
      .pipe(gulp.dest(tempWebFolder))
      .pipe(gulp.dest(sdcardFolder + '/web'));
});

gulp.task('web-images', function() {
  return gulp.src(baseFolder + 'web/media/*.png')
      .pipe(imagemin())
      .pipe(gulp.dest(tempWebFolder))
      .pipe(gulp.dest(sdcardFolder + '/web'));
});

gulp.task('display-images', function() {
  return gulp.src(baseFolder + 'display/image/*.png')
      .pipe(imagemin())
      .pipe(gulp.dest(tempWebFolder))
      .pipe(gulp.dest(sdcardFolder + '/display/images'));
});

gulp.task('display-file-copy', function() {
  return gulp.src(baseFolder + 'display/font/*.*')
      .pipe(gulp.dest(sdcardFolder + '/display/font'));
});

gulp.task(
    'default',
    gulp.series(
        'html', 'js', 'js-extra', 'css', 'web-images', 'display-images',
        'display-file-copy'));

gulp.task('serve', function() {
  browserSync.init({server: {baseDir: baseWebFolder}});
});

gulp.task('watchHtml', function() {
  gulp.watch(baseWebFolder + '**/*.html', reload)
});
gulp.task('watchJs', function() {
  gulp.watch(baseWebFolder + '**/*.js', reload)
});
gulp.task('watchCss', function() {
  gulp.watch(baseWebFolder + '**/*.css', reload)
});
gulp.task('watchPng', function() {
  gulp.watch(baseWebFolder + '**/*.png', reload)
});

// Static server
gulp.task(
    'webserver',
    gulp.parallel(['serve', 'watchHtml', 'watchJs', 'watchCss', 'watchPng']));
