import * as React from 'react';
import PropTypes from 'prop-types';
import Box from '@mui/material/Box';
import Collapse from '@mui/material/Collapse';
import IconButton from '@mui/material/IconButton';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import Paper from '@mui/material/Paper';
import KeyboardArrowDownIcon from '@mui/icons-material/KeyboardArrowDown';
import KeyboardArrowUpIcon from '@mui/icons-material/KeyboardArrowUp';

interface infoRow {
    id: number
    widget_capacity: number,
    pallet_capacity: number,
}

function createData(tableName: string) {

    const example1: infoRow = {
        id: 3,
        widget_capacity: 1234567489,
        pallet_capacity: 11091700,
    }

    const example2: infoRow = {
        id: 1,
        widget_capacity: 51354698,
        pallet_capacity: 123456,
    }

    return {
        tableName,
        info: [
            example1,
            example2,
        ],
    };
}

function Row(props: any) {
    const { row } = props;
    const [open, setOpen] = React.useState(false);

    return (
        <React.Fragment>
            <TableRow sx={{ '& > *': { borderBottom: 'unset' } }}>
                <TableCell>
                    <IconButton
                        aria-label="expand row"
                        size="small"
                        onClick={() => setOpen(!open)}
                    >
                        {open ? <KeyboardArrowUpIcon /> : <KeyboardArrowDownIcon />}
                    </IconButton>
                </TableCell>
                <TableCell component="th" scope="row">
                    {row.tableName}
                </TableCell>
            </TableRow>
            <TableRow>
                <TableCell style={{ paddingBottom: 0, paddingTop: 0 }} colSpan={6}>
                    <Collapse in={open} timeout="auto" unmountOnExit>
                        <Box sx={{ margin: 1 }}>
                            <Table size="small" aria-label="purchases">
                                <TableHead>
                                    <TableRow>
                                        <TableCell>widget_capacity</TableCell>
                                        <TableCell>pallet_capacity</TableCell>
                                        <TableCell align="right">ID</TableCell>
                                    </TableRow>
                                </TableHead>
                                <TableBody>
                                    {row.info.map((tr_info: infoRow) => (
                                        <TableRow key={tr_info.widget_capacity}>
                                            <TableCell component="th" scope="row">
                                                {tr_info.widget_capacity}
                                            </TableCell>
                                            <TableCell>{tr_info.pallet_capacity}</TableCell>
                                            <TableCell align="right">{tr_info.id}</TableCell>
                                        </TableRow>
                                    ))}
                                </TableBody>
                            </Table>
                        </Box>
                    </Collapse>
                </TableCell>
            </TableRow>
        </React.Fragment>
    );
}

Row.propTypes = {
    row: PropTypes.shape({
        info: PropTypes.arrayOf(
            PropTypes.shape({
                id: PropTypes.number.isRequired,
                pallet_capacity: PropTypes.number,
                widget_capacity: PropTypes.number,
            }),
        ).isRequired,
        tableName: PropTypes.string.isRequired,
    }).isRequired,
};

const rows = [
    createData('station_type'),
    createData('station'),
    createData('robot_type'),
    createData('robot'),
    createData('pallet'),
];

export default function CollapsibleTable() {
    return (
        <TableContainer component={Paper}>
            <Table aria-label="collapsible table">
                <TableHead>
                    <TableRow>
                        <TableCell />
                        <TableCell>Table Name</TableCell>
                    </TableRow>
                </TableHead>
                <TableBody>
                    {rows.map((row) => (
                        <Row key={row.tableName} row={row} />
                    ))}
                </TableBody>
            </Table>
        </TableContainer>
    );
}