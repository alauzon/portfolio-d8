<?php
/**
 * REST invoice model that is used for binding request data and returning the entity result
 */
class InvoiceRestModel
{
    public $leading_zeros;
    public $prefix;
    public $description;
    public $template;
    public $pay_limit;
    public $pay_status;

    public $customer = array(
        'customer_number' => null,
        'company_name' => null,
        'firstname' => null,
        'lastname' => null,
        'street' => null,
        'building_number' => null,
        'zipcode' => null,
        'city' => null,
        'state' => null,
        'country' => null,
        'coc_number' => null,
        'vat_number' => null,
        'description' => null,
    );

    /**
     * Invoices items that contains one array per item with the following fields:
     * description, quantity, unitcost, vat, weight
     *
     * Unitcost must be ex. VAT
     *
     * @var array
     */
    public $items = array();

    /**
     * Exchanges the specified data array
     *
     * @param array $data
     */
    public function exchangeArray($data)
    {
        $properties = get_object_vars($this);
        foreach ($properties as $property => $value) {
            if (isset($data[$property])) {
                $this->$property = $data[$property];
            }
        }
    }

    /**
     * Returns this object as array
     */
    public function getArrayCopy()
    {
        // Set invoice ID and node ID at the top
        $data = (array) $this;
        $invoiceId = null;
        $nid = null;

        foreach ($data as $key => $value) {
            if ('id' == $key) {
                $invoiceId = $value;
            } elseif ('nid' == $key) {
                $nid = $value;
            }
        }

        unset($data['id']);
        unset($data['nid']);

        $data = array_merge(array('nid' => $nid), $data);
        $data = array_merge(array('id' => $invoiceId), $data);

        // Set customer ID at the top
        $customerId = $data['customer']['id'];
        unset($data['customer']['id']);
        $customer = array_merge(array('id' => $customerId), $data['customer']);
        $data['customer'] = $customer;

        return $data;
    }

    /**
     * Maps the model to the specified node
     *
     * @param object $node
     */
    public function mapToNode($node)
    {
        // Set invoice properties
        $node->template = $this->template;
        $node->invoice_invoice_number_zerofill = $this->leading_zeros;
        $node->invoice_invoice_number_prefix = $this->prefix;
        $node->invoice_description = $this->description;
        $node->pay_limit = $this->pay_limit;
        $node->pay_status = $this->pay_status;

        // Set customer
        $customer = (object) $this->customer;

        $node->customer_number = $customer->customer_number;
        $node->company_name = $customer->company_name;
        $node->firstname = $customer->firstname;
        $node->lastname = $customer->lastname;
        $node->street = $customer->street;
        $node->building_number = $customer->building_number;
        $node->zipcode = $customer->zipcode;
        $node->city = $customer->city;
        $node->state = $customer->state;
        $node->country = $customer->country;
        $node->coc_number = $customer->coc_number;
        $node->vat_number = $customer->vat_number;
        $node->customer_description = $customer->description;
        $node->description = $customer->description;

        // Set items
        $node->invoice_items = $this->items;
    }

    /**
     * Maps the model from the specified node
     *
     * @param object $node
     */
    public function mapFromNode($node)
    {
        // Set node ID
        $this->nid = (int) $node->nid;

        // Set invoice properties
        $this->id = (int) $node->invoice['invoice_number'];
        $this->number =
            _invoice_get_formatted_invoice_number($node->invoice['formatted_invoice_number'], NULL, $node->created);
        $this->leading_zeros = (int) $node->invoice['invoice_number_zerofill'];
        $this->prefix = $node->invoice['invoice_number_prefix'];
        $this->description = $node->invoice['description'];
        $this->template = $node->invoice['template'];
        $this->pay_limit = (int) $node->invoice['pay_limit'];
        $this->pay_status = $node->invoice['pay_status'];

        $vatData = $node->invoice['vat'];
        $vatTotals = array();
        foreach ($vatData as $vat => $fields) {
            $vatTotals[] = array_merge(array('vat' => (float) $vat), $fields);
        }

        $this->vat_totals = $vatTotals;
        $this->vattotal = (float) $node->invoice['vattotal'];
        $this->extotal = (float) $node->invoice['extotal'];
        $this->inctotal = (float) $node->invoice['inctotal'];
        $this->formatted_vattotal = $node->invoice['formatted_vattotal'];
        $this->formatted_extotal = $node->invoice['formatted_extotal'];
        $this->formatted_inctotal = $node->invoice['formatted_inctotal'];
        $this->formatted_created = $node->invoice['formatted_created'];
        $this->created = date('Y-m-d H:i:s', $node->created);
        $this->changed = date('Y-m-d H:i:s', $node->changed);

        // Set customer
        $this->customer['id'] = (int) $node->customer['cid'];
        $this->customer['customer_number'] = (int) $node->customer['customer_number'];
        $this->customer['company_name'] = $node->customer['company_name'];
        $this->customer['firstname'] = $node->customer['firstname'];
        $this->customer['lastname'] = $node->customer['lastname'];
        $this->customer['street'] = $node->customer['street'];
        $this->customer['building_number'] = $node->customer['building_number'];
        $this->customer['zipcode'] = $node->customer['zipcode'];
        $this->customer['city'] = $node->customer['city'];
        $this->customer['state'] = $node->customer['state'];
        $this->customer['country'] = $node->customer['country'];
        $this->customer['coc_number'] = $node->customer['coc_number'];
        $this->customer['vat_number'] = $node->customer['vat_number'];
        $this->customer['description'] = $node->customer['description'];

        // Set items
        $this->items = array();
        foreach ($node->invoice_items as $item) {
            $item = array_merge(array('id' => $item['iid']), $item);
            unset($item['iid']);

            $item['id'] = (int) $item['id'];
            $item['quantity'] = (float) $item['quantity'];
            $item['unitcost'] = (float) $item['unitcost'];
            $item['vat'] = (float) $item['vat'];
            $item['weight'] = (int) $item['weight'];

            $this->items[] = $item;
        }
    }
}