<?php

/**
 * @file
 * HURDAT2 Parser class
 */

/**
 * Parses a given file as a HURDAT2 (c. 2012) file.
 */
class FeedsHURDAT2Parser extends FeedsParser {

    private $basin_map = array(
        'AL' => 'North Atlantic',
    );
    private $status_description = array(
        'TD' => 'Tropical Cyclone of Tropical Depression intensity (< 34 kt)',
        'TS' => 'Tropical Cyclone of Tropical Storm intensity (34-63 kt)',
        'HU' => 'Tropical Cyclone of Hurricane intensity (> 64 kt)',
        'EX' => 'Extratropical Cyclone (no wind speed threshold)',
        'SD' => 'Subtropical Cyclone of Subtropical Depression intensity (< 34 kt)',
        'SS' => 'Subtropical Cyclone of Subtropical Storm intensity (> 34 kt)',
        'LO' => 'Low Pressure, but not a Tropical Cyclone, a Subtropical Cyclone, nor an Extratropical Cyclone (no wind speed threshold)',
        'WV' => 'Tropical Wave (no wind speed threshold)',
        'DB' => 'Disturbance (no wind speed threshold)',
    );
    private $status_title = array(
        'TD' => 'Tropical Depression',
        'TS' => 'Tropical Storm',
        'HU' => 'Hurricane',
        'EX' => NULL,
        'SD' => NULL,
        'SS' => NULL,
        'LO' => NULL,
        'WV' => NULL,
        'DB' => NULL,
    );
    private $regex_header = '/^(?P<basin>..)(?P<nth>\d\d)(?P<year>\d{4}),(?P<name>[- A-Z]{16}),(?P<count>[ \d]{7}),$/';
    private $regex_data = '/^(?P<date>\d{8}), (?P<hh>\d{2})(?P<mm>\d{2}), (?P<status>..), (?P<latitude>[- \d]{2}\.\d)[NS], (?P<longitude>[- \d]{3}\.\d)[EW], (?P<intensity>[- \d]{3}), (?P<pressure>[- \d]{4}), (?P<ne34>[- \d]{4}), (?P<se34>[- \d]{4}), (?P<sw34>[- \d]{4}), (?P<nw34>[- \d]{4}), (?P<ne50>[- \d]{4}), (?P<se50>[- \d]{4}), (?P<sw50>[- \d]{4}), (?P<nw50>[- \d]{4}), (?P<ne64>[- \d]{4}), (?P<se64>[- \d]{4}), (?P<sw64>[- \d]{4}), (?P<nw64>[- \d]{4}),$/';

    /**
     * Implements FeedsParser::parse().
     */
    public function parse(FeedsSource $source, FeedsFetcherResult $fetcher_result) {
        $result = new FeedsParserResult();
        $lines = preg_split('/\R/', $fetcher_result->getRaw());
        $lines_count = count($lines);
        $i = 0;
        while ($i < $lines_count and $lines[$i][0] === '<') {
            // skip HTML lines, if necessary
            $i++;
        }
        while ($i < $lines_count) {
            if (!preg_match($this->regex_header, $lines[$i], $header_matches)) {
                break;
            }
            $i++;
            $remaining_data_count = (int) $header_matches['count'];

            $basin_code = $header_matches['basin'];
            $nth = $header_matches['nth'];
            $year = $header_matches['year'];
            $name = ltrim($header_matches['name'], ' ');

            $basin_name = $this->basin_map[$basin_code];
            $stormid = $basin_code . $nth . $year;

            $dates_start = NULL;
            $dates_end = NULL;

            $max_status = NULL;
            $max_intensity = -1;
            $max_intensity_status = NULL;
            $max_latitude = -91;
            $min_latitude = 91;
            $max_longitude = -181;
            $min_longitude = 181;

            $data = array();
            $track = '';
            $trackwkt = array();

            while ($i < $lines_count and $remaining_data_count > 0) {
                if (!preg_match($this->regex_data, $lines[$i], $data_matches)) {
                    break;
                }
                $i++;
                $remaining_data_count--;

                $date = $data_matches['date'];
                if (is_null($dates_start) || $date < $dates_start) {
                    $dates_start = $date;
                }
                if (is_null($dates_end) || $date > $dates_end) {
                    $dates_end = $date;
                }

                $status = $data_matches['status'];
                if ($status == 'HU' || $max_status == 'HU') {
                    $max_status = 'HU';
                }
                elseif ($status == 'TS' || $max_status == 'TS') {
                    $max_status = 'TS';
                }
                elseif ($status == 'TD' || $max_status == 'TD') {
                    $max_status = 'TD';
                }

                $intensity = (int) $data_matches['intensity'];
                if ($intensity > $max_intensity) {
                    $max_intensity = $intensity;
                    $max_intensity_status = $status;
                }

                $latitude = (int) ltrim($data_matches['latitude']);
                if ($latitude > $max_latitude) {
                    $max_latitude = $latitude;
                }
                if ($latitude < $min_latitude) {
                    $min_latitude = $latitude;
                }

                $longitude = -1 * (int) ltrim($data_matches['longitude']);
                if ($longitude > $max_longitude) {
                    $max_longitude = $longitude;
                }
                if ($longitude < $min_longitude) {
                    $min_longitude = $longitude;
                }

                $trackwkt[] = $longitude . ' ' . $latitude;

                $data[] = array(
                    'date' => $date,
                    'hh' => $data_matches['hh'],
                    'mm' => $data_matches['mm'],
                    'status' => $status,
                    'latitude' => $latitude,
                    'longitude' => $longitude,
                    'intensity' => $intensity,
                    'pressure' => $data_matches['pressure'],
                    'ne34' => $data_matches['ne34'],
                    'se34' => $data_matches['se34'],
                    'sw34' => $data_matches['sw34'],
                    'nw34' => $data_matches['nw34'],
                    'ne50' => $data_matches['ne50'],
                    'se50' => $data_matches['se50'],
                    'sw50' => $data_matches['sw50'],
                    'nw50' => $data_matches['nw50'],
                    'ne64' => $data_matches['ne64'],
                    'se64' => $data_matches['se64'],
                    'sw64' => $data_matches['sw64'],
                    'nw64' => $data_matches['nw64'],
                );
                $track .=
                        $data_matches['date'] .
                        'T' .
                        $data_matches['hh'] .
                        ':' .
                        $data_matches['mm'] .
                        ',' .
                        $status .
                        ',' .
                        $latitude .
                        ',' .
                        $longitude .
                        ',' .
                        $intensity .
                        ',' .
                        $data_matches['pressure'] .
                        ',' .
                        $data_matches['ne34'] .
                        ',' .
                        $data_matches['se34'] .
                        ',' .
                        $data_matches['sw34'] .
                        ',' .
                        $data_matches['nw34'] .
                        ',' .
                        $data_matches['ne50'] .
                        ',' .
                        $data_matches['se50'] .
                        ',' .
                        $data_matches['sw50'] .
                        ',' .
                        $data_matches['nw50'] .
                        ',' .
                        $data_matches['ne64'] .
                        ',' .
                        $data_matches['se64'] .
                        ',' .
                        $data_matches['sw64'] .
                        ',' .
                        $data_matches['nw64'] .
                        "\n";
            }

            if (is_null($max_status)) {
                $max_status = $max_intensity_status;
            }
            if (is_null($max_status)) {
                $status_title = NULL;
                $status_description = NULL;
            }
            else {
                $status_title = $this->status_title[$max_status];
                $status_description = $this->status_description[$max_status];
            }

            $title = '';
            if (!is_null($status_title)) {
                $title .= $status_title . ' ';
            }
            $title .= ucwords(strtolower($name));
            $title .= ' (' . $stormid . ')';

            $bbox = array($min_longitude, $min_latitude, $max_longitude, $max_latitude);
            $trackwkts = (count($trackwkt) > 0) ? 'LINESTRING (' . implode(', ', $trackwkt) . ')' : NULL;

            $result->items[] = array(
                'stormid' => $stormid,
                'title' => $title,
                'basin' => $basin_name,
                'basin_code' => $basin_code,
                'nth' => $nth,
                'year' => $year,
                'name' => $name,
                'date_start' => $dates_start,
                'date_end' => $dates_end,
                'status' => $status_title,
                'status_description' => $status_description,
                'status_code' => $max_status,
                'bbox' => $bbox,
                'trackwkt' => $trackwkts,
                'data' => $data,
                'track' => $track,
            );

            drupal_debug($stormid, 'stormid');
        }
        drupal_debug($i, 'i');
        return $result;
    }

    /**
     * Implements FeedsParser::getMappingSources().
     */
    public function getMappingSources() {
        return array(
            'stormid' => array(
                'name' => t('Storm ID'),
                'description' => t('Unique identifier for storm.'),
            ),
            'title' => array(
                'name' => t('Title'),
                'description' => t('Title for storm node.'),
            ),
            'basin' => array(
                'name' => t('Basin'),
                'description' => t('Basin of storm.'),
            ),
            'basin_code' => array(
                'name' => t('Basin Code'),
                'description' => t('Abbreviated basin.'),
            ),
            'nth' => array(
                'name' => t('Storm number'),
                'description' => t('Storm number for that year.'),
            ),
            'year' => array(
                'name' => t('Year'),
                'description' => t('Year of storm.'),
            ),
            'name' => array(
                'name' => t('Name'),
                'description' => t('Name of storm.'),
            ),
            'date_start' => array(
                'name' => t('Date: Start'),
                'description' => t('First date for storm.'),
            ),
            'date_end' => array(
                'name' => t('Date: End'),
                'description' => t('Last date for storm.'),
            ),
            'status' => array(
                'name' => t('Status'),
                'description' => t('Label for maximum strength of storm.'),
            ),
            'status_description' => array(
                'name' => t('Status Description'),
                'description' => t('Description for maximum strength of storm.'),
            ),
            'status_code' => array(
                'name' => t('Status Code'),
                'description' => t('Abbreviated status.'),
            ),
            'bbox' => array(
                'name' => t('Bounding Box'),
                'description' => t('Geographic extent of storm.'),
            ),
            'trackwkt' => array(
                'name' => t('Best Track WKT'),
                'description' => t('Best track in WKT form.'),
            ),
            'data' => array(
                'name' => t('Data'),
                'description' => t('List of storm data points.'),
            ),
            'track' => array(
                'name' => t('Track'),
                'description' => t('Storm track.'),
            ),
                ) + parent::getMappingSources();
    }

}
