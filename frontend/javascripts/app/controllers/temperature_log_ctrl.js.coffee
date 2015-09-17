window.ChaiBioTech.ngApp.controller 'TemperatureLogCtrl', [
  '$scope'
  '$stateParams'
  'Status'
  'ChartData'
  'Experiment'
  'SecondsDisplay'
  '$interval'
  '$rootScope'
  ($scope, $stateParams, Status, ChartData, Experiment, SecondsDisplay, $interval, $rootScope) ->

    hasStatusData = false
    hasExperiment = false
    hasInit = false
    dragScroll = angular.element('.chart-drag-scroll')

    Experiment.get id: $stateParams.id, (data) ->
      $scope.experiment = data.experiment
      hasExperiment = true
      $scope.init()

    $scope.$watch ->
      Status.getData()
    , (val) ->
      if val
        hasStatusData = true
        $scope.isCurrentExperiment = parseInt(val.experimentController?.expriment?.id) is parseInt($stateParams.id)
        if $scope.isCurrentExperiment and ($scope.scrollState >= 1 || $scope.scrollState is 'FULL') and (val.experimentController?.machine.state is 'LidHeating' || val.experimentController?.machine.state is 'Running')
          $scope.autoUpdateTemperatureLogs()
        else
          $scope.stopInterval()

        $scope.init()

    $scope.init = ->

      return if !hasStatusData or !hasExperiment or hasInit

      hasInit = true

      Status.startSync()

      $scope.$on '$destroy', ->
        Status.stopSync()
        $scope.stopInterval()

      $scope.isCurrentExperiment = false

      $scope.temperatureLogs = []
      $scope.temperatureLogsCache = []
      $scope.calibration = 800
      $scope.updateInterval = null


      $scope.resolutionOptions = []
      $scope.resolutionOptionsIndex = 0

      Experiment
      .getTemperatureData($stateParams.id, resolution: 1000)
      .success (data) =>
        hasTemperatureLogs = true
        if data.length > 0
          $scope.temperatureLogsCache = angular.copy data
          $scope.temperatureLogs = angular.copy data
          $scope.greatest_elapsed_time = Math.floor data[data.length - 1].temperature_log.elapsed_time
          $scope.initResolutionOptions()
          $scope.resolutionOptionsIndex = $scope.resolutionOptions.length-1
          $scope.resolution = $scope.resolutionOptions[$scope.resolutionOptionsIndex]
          $scope.updateYScale()
          $scope.updateScrollWidth()
          $scope.resizeTemperatureLogs()
          $scope.updateResolution()
          $scope.updateData()
        else
          $scope.data = []
          $scope.autoUpdateTemperatureLogs()

    $scope.zoomOut = ->
      if $scope.resolutionOptions.length > 0 and $scope.resolutionOptionsIndex isnt ($scope.resolutionOptions.length - 1)
        $scope.resolutionOptionsIndex += 1

      $scope.resolution = $scope.resolutionOptions[$scope.resolutionOptionsIndex]
      $scope.updateResolution()

    $scope.zoomIn = ->
      if $scope.resolutionOptionsIndex isnt 0 and $scope.resolutionOptions.length
        $scope.resolutionOptionsIndex -= 1
        $scope.resolution = $scope.resolutionOptions[$scope.resolutionOptionsIndex]
        $scope.updateResolution()

    $scope.updateData = ->
      if $scope.temperatureLogsCache?.length > 0
        left_et_limit = $scope.temperatureLogsCache[$scope.temperatureLogsCache.length-1].temperature_log.elapsed_time - ($scope.resolution*1000)

        maxScroll = 0
        for temp_log in $scope.temperatureLogs
          if temp_log.temperature_log.elapsed_time <= left_et_limit
            ++ maxScroll
          else
            break

        scrollState = Math.round (if $scope.scrollState is 'FULL' then 1 else $scope.scrollState) * maxScroll
        if $scope.scrollState < 0 then scrollState = 0
        if $scope.scrollState > 1 then scrollState = maxScroll
        left_et = $scope.temperatureLogs[scrollState].temperature_log.elapsed_time

        right_et = left_et + ($scope.resolution*1000)

        data = _.select $scope.temperatureLogs, (temp_log) ->
          et = temp_log.temperature_log.elapsed_time
          et >= left_et and et <= right_et

        $scope.updateChart data

    $scope.updateYScale = ->
      scales = _.map $scope.temperatureLogsCache, (temp_log) ->
        temp_log = temp_log.temperature_log
        greatest = Math.max.apply Math, [
          parseFloat temp_log.lid_temp
          parseFloat temp_log.heat_block_zone_1_temp
          parseFloat temp_log.heat_block_zone_2_temp
        ]
        greatest

      max_scale = Math.max.apply Math, scales
      $scope.options.axes.y.max = Math.ceil(max_scale/10)*10

    $scope.updateDragScrollWidthAttr = ->
      dragScrollWidth = dragScroll.css('width').replace 'px', ''
      w = ($scope.greatest_elapsed_time / 1000) / $scope.resolution * dragScrollWidth
      dragScroll.attr 'width', Math.round w

    $scope.updateScrollWidth = ->

      if $scope.temperatureLogsCache.length > 0
        $scope.widthPercent = $scope.resolution*1000/$scope.greatest_elapsed_time
        if $scope.widthPercent > 1
          $scope.widthPercent = 1
      else
        $scope.widthPercent = 1

      angular.element('.scrollbar').css width: "#{$scope.widthPercent*100}%"
      $rootScope.$broadcast 'scrollbar:width:changed'

    $scope.resizeTemperatureLogs = ->
      resolution = $scope.resolution
      if $scope.resolution> $scope.greatest_elapsed_time/1000 then resolution = $scope.greatest_elapsed_time/1000
      chunkSize = Math.round resolution / $scope.calibration
      temperature_logs = angular.copy $scope.temperatureLogsCache
      chunked = _.chunk temperature_logs, chunkSize
      averagedLogs = _.map chunked, (chunk) ->
        i = Math.floor(chunk.length/2)
        return chunk[i]

      averagedLogs.unshift temperature_logs[0]
      averagedLogs.push temperature_logs[temperature_logs.length-1]
      $scope.temperatureLogs = averagedLogs

    $scope.updateResolution = =>
      if $scope.temperatureLogsCache?.length > 0
        $scope.resizeTemperatureLogs()
        $scope.updateScrollWidth()
        $scope.updateDragScrollWidthAttr()
        $scope.updateData()

    $scope.$watch 'widthPercent', ->
      if $scope.widthPercent is 1 and $scope.isCurrentExperiment
        $scope.autoUpdateTemperatureLogs()

    $scope.$watch 'scrollState', ->
      if $scope.scrollState and $scope.temperatureLogs and $scope.data
        $scope.updateData()

        if ($scope.scrollState >= 1 or $scope.scrollState is 'FULL') and $scope.isCurrentExperiment
          $scope.autoUpdateTemperatureLogs()
        else
          $scope.stopInterval()

    $scope.$on 'experiment:started', (e, expId) ->
      if parseInt(expId) is parseInt($stateParams.id)
        $scope.scrollState = 1
        $scope.isCurrentExperiment = true
        $scope.autoUpdateTemperatureLogs()

    $scope.$watch 'dragScroll', (val) ->
      val = $scope.resolution*val/($scope.greatest_elapsed_time/1000)
      $scope.scrollState += val*-1

    $scope.updateChart = (temperature_logs) ->
      $scope.data = ChartData.temperatureLogs(temperature_logs).toN3LineChart()

    updateFunc = ->
      Experiment
      .getTemperatureData($stateParams.id, resolution: 1000)
      .success (data) ->
        if data.length > 0
          $scope.temperatureLogsCache = angular.copy data
          $scope.temperatureLogs = angular.copy data
          $scope.greatest_elapsed_time = Math.floor data[data.length - 1].temperature_log.elapsed_time
          $scope.initResolutionOptions()
          if !$scope.resolution or $scope.greatest_elapsed_time/1000 < 60 * 5
            $scope.resolutionOptionsIndex = $scope.resolutionOptions.length-1
            $scope.resolution = $scope.resolutionOptions[$scope.resolutionOptionsIndex]
          $scope.updateYScale()
          $scope.updateScrollWidth()
          $scope.resizeTemperatureLogs()
          $scope.updateResolution()
          $scope.updateData()

    $scope.autoUpdateTemperatureLogs = =>
      if !$scope.updateInterval
        updateFunc()
        $scope.updateInterval = $interval updateFunc, 10000

    $scope.stopInterval = =>
      $interval.cancel $scope.updateInterval if $scope.updateInterval
      $scope.updateInterval = null

    $scope.initResolutionOptions = ->
      $scope.resolutionOptions = []
      options = [
        30
        60
        60 * 2
        60 * 3
        60 * 5
        60 * 10
        60 * 20
        60 * 30
        60 * 60
        60 * 60 * 24
      ]

      for opt in options
        if opt < $scope.greatest_elapsed_time/1000
          $scope.resolutionOptions.push opt

      $scope.resolutionOptions.push Math.floor $scope.greatest_elapsed_time/1000

    $scope.options = {
      axes: {
        x: {
          key: 'elapsed_time',
          ticksFormatter: (t) -> SecondsDisplay.display2(t)
          ticks: 8
        },
        y: {
          key: 'heat_block_zone_temp'
          type: 'linear'
          min: 0
          max: 0
        }
      },
      margin: {
        left: 30
      },
      series: [
        {y: 'heat_block_zone_temp', color: 'steelblue'},
        {y: 'lid_temp', color: 'lightsteelblue'}
      ],
      lineMode: 'linear',
      thickness: '2px',
      tension: 0.7,
      tooltip: {
        mode: 'scrubber',
        formatter: (x, y, series) ->
          if series.y is 'lid_temp'
            return "#{SecondsDisplay.display2(x)} | Lid Temp: #{y}"
          else if series.y is 'heat_block_zone_temp'
            return "#{SecondsDisplay.display2(x)} | Heat Block Zone Temp: #{y}"
          else
            return ''
      },
      drawLegend: false,
      drawDots: false,
      hideOverflow: false,
      columnsHGap: 5
    }

]